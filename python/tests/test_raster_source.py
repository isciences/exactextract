import numpy as np
import pytest

from exactextract.raster import (
    GDALRasterSource,
    RasterioRasterSource,
    XArrayRasterSource,
)


@pytest.fixture()
def global_half_degree(tmp_path):
    gdal = pytest.importorskip("osgeo.gdal")
    osr = pytest.importorskip("osgeo.osr")

    srs = osr.SpatialReference()
    srs.ImportFromEPSG(4326)

    fname = str(tmp_path / "test.tif")

    nx = 720
    ny = 360

    drv = gdal.GetDriverByName("GTiff")
    ds = drv.Create(fname, nx, ny, eType=gdal.GDT_Int32)
    gt = (-180.0, 0.5, 0.0, 90.0, 0.0, -0.5)
    ds.SetGeoTransform(gt)
    ds.SetSpatialRef(srs)
    band = ds.GetRasterBand(1)
    band.SetNoDataValue(6)

    data = np.arange(nx * ny).reshape(ny, nx)
    band.WriteArray(data)

    ds = None

    return fname


@pytest.fixture()
def scaled_temperature(tmp_path):
    gdal = pytest.importorskip("osgeo.gdal")

    fname = str(tmp_path / "d2m.tif")

    nx = 2
    ny = 2

    drv = gdal.GetDriverByName("GTiff")
    ds = drv.Create(fname, nx, ny, eType=gdal.GDT_Int16)
    gt = (1, 1, 0, 2, 0, -1)
    ds.SetGeoTransform(gt)

    band = ds.GetRasterBand(1)

    data = np.array([[17650, 18085], [19127, 19428]], dtype=np.int16)
    band.SetScale(0.001571273180860454)
    band.SetOffset(250.47270984680802)

    band.WriteArray(data)

    ds = None

    return fname


@pytest.mark.parametrize(
    "Source", (GDALRasterSource, RasterioRasterSource, XArrayRasterSource)
)
def test_basic_read(global_half_degree, Source):
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

    assert "WGS_1984" in src.srs_wkt()


@pytest.mark.parametrize(
    "Source", (GDALRasterSource, RasterioRasterSource, XArrayRasterSource)
)
def test_scaled_raster(scaled_temperature, Source):
    try:
        src = Source(scaled_temperature, 1)
    except ModuleNotFoundError as e:
        pytest.skip(str(e))

    window = src.read_window(0, 0, 2, 2)

    assert window[0, 0] == pytest.approx(278.2057, rel=1e-3)
