#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pathlib
import pytest

from osgeo import gdal

from exactextract import GDALRasterWrapper
from _exactextract import parse_raster_descriptor

# Path to data
data_path = (pathlib.Path(__file__).parent / 'data').resolve()
simple_tif_path = pathlib.Path(data_path, 'simple_raster.tif')
assert simple_tif_path.is_file()
simple_netcdf_path = pathlib.Path(data_path, 'simple_raster.nc')
assert simple_netcdf_path.is_file()


class TestGDALRasterWrapper():

    def test_default_filename(self):
        dsw = GDALRasterWrapper(str(simple_tif_path))
        assert dsw is not None

    def test_default_ds(self):
        ds = gdal.OpenEx(str(simple_tif_path))
        dsw = GDALRasterWrapper(ds)
        assert dsw is not None
        dsw = None
        ds = None

    def test_default_filename_netcdf(self):
        dsw = GDALRasterWrapper('NETCDF:%s:Band1' % str(simple_netcdf_path))
        assert dsw is not None
        dsw = GDALRasterWrapper('NETCDF:"%s":Band1' % str(simple_netcdf_path))
        assert dsw is not None

    def test_custom_band(self):
        dsw = GDALRasterWrapper(str(simple_tif_path), band_idx=2)
        assert dsw is not None
        dsw = None

    def test_invalid_band(self):
        with pytest.raises(ValueError):
            GDALRasterWrapper(str(simple_tif_path), band_idx=0)
        with pytest.raises(ValueError):
            GDALRasterWrapper(str(simple_tif_path), band_idx=3)

    def test_invalid_band_netcdf(self):
        with pytest.raises(RuntimeError):
            GDALRasterWrapper('NETCDF:%s:BadBand' % str(simple_netcdf_path))

    def test_descriptor(self):
        assert parse_raster_descriptor('temp:NETCDF:outputs.nc:tmp2m') == (
            'temp', 'NETCDF:outputs.nc:tmp2m', 1)
        assert parse_raster_descriptor('temp2:temperature.tif[4]') == (
            'temp2', 'temperature.tif', 4)

    def test_valid_descriptor(self):
        dsw = GDALRasterWrapper.from_descriptor(str(simple_tif_path))
        assert dsw is not None
        dsw = GDALRasterWrapper.from_descriptor(str(simple_tif_path) + '[2]')
        assert dsw is not None
        dsw = GDALRasterWrapper.from_descriptor('temp:NETCDF:%s:Band2' %
                                                str(simple_netcdf_path))
        assert dsw is not None

    def test_invalid_descriptor(self):
        # Layer name not given, and '0' is not a valid layer
        with pytest.raises(ValueError):
            GDALRasterWrapper.from_descriptor(str(simple_tif_path) + '[3]')
        with pytest.raises(ValueError):
            GDALRasterWrapper.from_descriptor(
                str(simple_tif_path) + '[bad_input]')
        # NetCDF without dataset name
        with pytest.raises(RuntimeError):
            GDALRasterWrapper.from_descriptor('temp:NETCDF:%s' %
                                              str(simple_netcdf_path))
