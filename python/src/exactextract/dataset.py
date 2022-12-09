#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from __future__ import annotations

import pathlib
from typing import Optional, Union
from osgeo import gdal, ogr

from _exactextract import GDALDatasetWrapper as _GDALDatasetWrapper, parse_dataset_descriptor

from .utils import get_ds_path, get_field_names


class GDALDatasetWrapper(_GDALDatasetWrapper):
    """ Binding class around exactextract GDALDatasetWrapper """

    def __init__(self,
                 filename_or_ds: Union[str, pathlib.Path, gdal.Dataset,
                                       ogr.DataSource],
                 layer_name: Optional[str] = None,
                 layer_idx: Optional[int] = None,
                 field_name: Optional[str] = None,
                 field_idx: Optional[int] = None):
        """
        Create exactextract GDALDatasetWrapper object from Python OSGeo Dataset
        object or from file path.

        Args:
            filename_or_ds (Union[str, pathlib.Path, gdal.Dataset, ogr.DataSource]): File path or OSGeo Dataset / DataSource
            layer_name (Optional[str], optional): Optional name of layer. Defaults to None.
            layer_idx (Optional[int], optional): Optional layer index. Defaults to None.
            field_name (Optional[str], optional): Optional ID field name. Defaults to None.
            field_idx (Optional[int], optional): Optional ID field index. Defaults to None.

        Raises:
            RuntimeError: If the provided dataset path is not found.
            RuntimeError: If the dataset could not be opened.
            RuntimeError: If the dataset layer could not be opened.
            RuntimeError: If the field names for the layer could not be read.
            ValueError: If the provided field name is invalid
        """
        # Get file path based on input, filename or dataset
        if isinstance(filename_or_ds, (gdal.Dataset, ogr.DataSource)):
            path = pathlib.Path(get_ds_path(filename_or_ds))
        else:
            path = pathlib.Path(filename_or_ds)

        if not str(path).startswith('/vsi'):
            # Assert the path exists and resolve the full path
            if not path.exists():
                raise RuntimeError('File path not found: %s' % str(path))
            path = path.resolve()

        # Name of the dataset
        this_ds_name = str(path)

        try:
            # Open the dataset
            tmp_ds: Optional[gdal.Dataset] = gdal.OpenEx(this_ds_name)
            if tmp_ds is None:
                raise RuntimeError('Failed to open dataset!')

            # Load the layer of interest
            layer: Optional[ogr.Layer]
            if layer_name is not None:
                layer = tmp_ds.GetLayerByName(layer_name)
            elif layer_idx is not None:
                layer = tmp_ds.GetLayerByIndex(layer_idx)
            else:
                layer = tmp_ds.GetLayer()
            if layer is None:
                raise RuntimeError('Unable to open layer!')

            # Name of the layer
            this_layer_name: str = layer.GetName()

            # Get list of fields for this layer
            # and determine field of interest
            field_names = get_field_names(layer)
            if not len(field_names):
                raise RuntimeError('Could not get field names for layer: %s' %
                                   this_layer_name)
            if field_name is not None:
                if field_name not in field_names:
                    raise ValueError(
                        'Did not find field name \'%s\' in layer! Got: %s' %
                        (field_name, repr(field_names)))
                this_field_name = field_name
            elif field_idx is not None:
                this_field_name = field_names[field_idx]
            else:
                this_field_name = field_names[0]
        finally:
            # Clean up
            layer = None
            tmp_ds = None

        # Put it all together
        super().__init__(this_ds_name, this_layer_name, this_field_name)

    @classmethod
    def from_descriptor(cls,
                        desc: str,
                        field_name: Optional[str] = None,
                        field_idx: Optional[int] = None) -> GDALDatasetWrapper:
        """
        Create GDALDatasetWrapper object from exactextract dataset descriptor (like what the CLI takes)

        Args:
            desc (str): exactextract stat descriptor (example: 'test.gpkg[layer_name]')
            field_name (Optional[str], optional): Optional ID field name. Defaults to None.
            field_idx (Optional[int], optional): Optional ID field index. Defaults to None.

        Returns:
            GDALDatasetWrapper: exactextract GDALDatasetWrapper object
        """
        filename, layer_name = parse_dataset_descriptor(desc)
        return cls(filename,
                   layer_name=layer_name,
                   field_name=field_name,
                   field_idx=field_idx)
