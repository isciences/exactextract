#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pytest

from exactextract import GDALRasterSource, RasterioRasterSource


@pytest.fixture()
def global_half_degree(tmp_path):
    from osgeo import gdal

    fname = str(tmp_path / "test.tif")

    drv = gdal.GetDriverByName("GTiff")
    ds = drv.Create(fname, 720, 360)
    gt = (-180.0, 0.5, 0.0, 90.0, 0.0, -0.5)
    ds.SetGeoTransform(gt)
    band = ds.GetRasterBand(1)
    band.SetNoDataValue(6)
    ds = None

    return fname


@pytest.mark.parametrize("Source", (GDALRasterSource, RasterioRasterSource))
def test_gdal_raster(global_half_degree, Source):
    try:
        src = Source(global_half_degree, 1)
    except ModuleNotFoundError as e:
        pytest.skip(str(e))

    assert src.res() == (0.50, 0.50)
    assert src.extent() == pytest.approx((-180, -90, 180, 90))
    assert src.nodata_value() == 6

    assert src.read_window(0, 0, 10, 10).shape == (10, 10)
