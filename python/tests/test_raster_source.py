#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import numpy as np
import pytest

from exactextract import GDALRasterSource, RasterioRasterSource, XArrayRasterSource


@pytest.fixture()
def global_half_degree(tmp_path):
    from osgeo import gdal

    fname = str(tmp_path / "test.nc")

    nx = 720
    ny = 360

    drv = gdal.GetDriverByName("NetCDF")
    ds = drv.Create(fname, nx, ny, eType=gdal.GDT_Int32)
    gt = (-180.0, 0.5, 0.0, 90.0, 0.0, -0.5)
    ds.SetGeoTransform(gt)
    band = ds.GetRasterBand(1)
    band.SetNoDataValue(6)

    data = np.arange(nx * ny).reshape(ny, nx)
    band.WriteArray(data)

    ds = None

    return fname


@pytest.mark.parametrize(
    "Source", (GDALRasterSource, RasterioRasterSource, XArrayRasterSource)
)
def test_gdal_raster(global_half_degree, Source):
    try:
        src = Source(global_half_degree, 1)
    except ModuleNotFoundError as e:
        pytest.skip(str(e))

    assert src.res() == (0.50, 0.50)
    assert src.extent() == pytest.approx((-180, -90, 180, 90))

    window = src.read_window(4, 5, 2, 3)

    assert window.shape == (3, 2)
    np.testing.assert_array_equal(
        window.astype(np.float64),
        np.array([[3604, 3605], [4324, 4325], [5044, 5045]], np.float64),
    )

    if Source != XArrayRasterSource:
        assert src.nodata_value() == 6
        assert window.dtype == np.int32
