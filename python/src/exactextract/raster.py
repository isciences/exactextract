#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import annotations

import pathlib
from typing import Optional, Tuple, Union
from osgeo import gdal

from _exactextract import GDALRasterWrapper as _GDALRasterWrapper, parse_raster_descriptor

from .utils import get_ds_path


class GDALRasterWrapper(_GDALRasterWrapper):
    """ Binding class around exactextract GDALRasterWrapper """

    def __init__(self,
                 filename_or_ds: Union[str, pathlib.Path, gdal.Dataset],
                 band_idx: int = 1, nc_file: bool = False):
        """
        Create exactextract GDALRasterWrapper object from Python OSGeo Dataset
        object or from file path.

        Args:
            filename_or_ds (Union[str, pathlib.Path, gdal.Dataset]): File path or OSGeo Dataset / DataSource
            band_idx (int, optional): Raster band index. Defaults to 1.
            nc_file (boolean, optional): Flag to indicate input string is netcdf format to call specific variable

        Raises:
            ValueError: If raster band index is <= 0
            RuntimeError: If path to dataset is not found.
            RuntimeError: If dataset could not be opened.
            ValueError: If raster band index is invalid.
        """
        # Sanity check inputs, set default band_idx
        if band_idx is not None and band_idx <= 0:
            raise ValueError('Raster band index starts from 1!')
        elif band_idx is None:
            band_idx = 1
        # if block to check whether user specified netcdf raster
        # or raster data from a tif file
        if(nc_file):
            # Check to see if expected netcdf gdal subdataset string indicates netcdf file
            nc_format = filename_or_ds.split(":")[0]
            if(nc_format != "NETCDF"):
                raise RuntimeError('Netcdf file is not proper format for GDAL. Returned as : %s' % str(nc_format))
            else:
                # String here indicates the raster being sliced from netcdf file
                this_ds_name = filename_or_ds
        else:
            # Get file path based on input, filename or dataset
            if isinstance(filename_or_ds, gdal.Dataset):
                path = pathlib.Path(get_ds_path(filename_or_ds))
            else:
                path = pathlib.Path(filename_or_ds)

            # Assert the path exists and resolve the full path
            if not path.exists():
                raise RuntimeError('File path not found: %s' % str(path))
            path = path.resolve()

            # Name of the dataset
            this_ds_name = str(path)

        # Open the dataset and load the layer of interest
        try:
            tmp_ds: Optional[gdal.Dataset] = gdal.Open(this_ds_name)
            if tmp_ds is None:
                raise RuntimeError('Failed to open dataset!')

            if band_idx > tmp_ds.RasterCount:
                raise ValueError('Band index is greater than raster count!')
        finally:
            # Clean up
            tmp_ds = None

        # Put it all together
        super().__init__(this_ds_name, band_idx)

    @classmethod
    def from_descriptor(cls, desc: str) -> Tuple[str, GDALRasterWrapper]:
        """
        Create GDALRasterWrapper object from exactextract raster descriptor (like what the CLI takes)

        Args:
            desc (str): exactextract stat descriptor (example: 'temp:NETCDF:outputs.nc:tmp2m', 'temp:temperature.tif[4]')

        Returns:
            Tuple[str, GDALRasterWrapper]: Raster name + exactextract GDALRasterWrapper object
        """
        name, filename, band_idx = parse_raster_descriptor(desc)
        return name, cls(filename, band_idx)
