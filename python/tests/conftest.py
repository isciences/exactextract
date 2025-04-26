import pytest

from exactextract.raster import NumPyRasterSource

try:
    from osgeo import gdal

    gdal.UseExceptions()
except ImportError:
    pass


@pytest.fixture()
def np_raster_source():
    np = pytest.importorskip("numpy")

    mat = np.arange(100).reshape(10, 10)

    return NumPyRasterSource(mat, 0, 0, 10, 10)


@pytest.fixture()
def square_features():
    return [
        {
            "id": "0",
            "geometry": {
                "type": "Polygon",
                "coordinates": [
                    [[0.5, 0.5], [2.5, 0.5], [2.5, 1.5], [0.5, 1.5], [0.5, 0.5]]
                ],
            },
        },
        {
            "id": "1",
            "geometry": {
                "type": "Polygon",
                "coordinates": [[[4, 4], [5, 4], [5, 5], [4, 5], [4, 4]]],
            },
        },
    ]
