import math
import pytest
import numpy as np

from exactextract import *


def make_square_raster(n):
    values = np.arange(1, n * n + 1).reshape(n, n)
    return NumPyRasterSource(values, 0, 0, n, n)


def make_rect(xmin, ymin, xmax, ymax, fid=1):
    return {
        "id": fid,
        "geometry": {
            "type": "Polygon",
            "coordinates": [
                [[xmin, ymin], [xmax, ymin], [xmax, ymax], [xmin, ymax], [xmin, ymin]]
            ],
        },
    }


@pytest.mark.parametrize(
    "stat,expected",
    [
        ("count", 4),
        ("mean", 5),
        ("median", 5),
        ("min", 1),
        ("max", 9),
        ("mode", 5),
        ("majority", 5),
        ("minority", 1),
        # TODO: add quantiles
        ("variety", 9),
        ("variance", 5),
        ("stdev", math.sqrt(5)),
        ("coefficient_of_variation", math.sqrt(5) / 5),
    ],
)
def test_basic_stats(stat, expected):
    rast = NumPyRasterSource(np.arange(1, 10).reshape(3, 3))
    square = JSONFeatureSource(make_rect(0.5, 0.5, 2.5, 2.5))

    assert exact_extract(rast, square, stat)[0]["properties"][stat] == pytest.approx(
        expected
    )


def test_multiple_stats():
    rast = NumPyRasterSource(np.arange(1, 10).reshape(3, 3))
    square = JSONFeatureSource(make_rect(0.5, 0.5, 2.5, 2.5))

    assert exact_extract(rast, square, ("min", "max", "mean"))[0]["properties"] == {
        "min": 1,
        "max": 9,
        "mean": 5,
    }


@pytest.mark.parametrize("stat", ("mean", "sum", "stdev", "variance"))
def test_weighted_stats_equal_weights(stat):
    rast = NumPyRasterSource(np.arange(1, 10).reshape(3, 3))
    weights = NumPyRasterSource(np.full((3, 3), 1))
    square = JSONFeatureSource(make_rect(0.5, 0.5, 2.5, 2.5))

    weighted_stat = f"weighted_{stat}"

    results = exact_extract(rast, square, [stat, weighted_stat], weights=weights)[0][
        "properties"
    ]

    assert results[stat] == results[weighted_stat]


@pytest.mark.parametrize(
    "stat,expected",
    [
        ("weighted_mean", (0.25 * 7 + 0.5 * 8 + 0.25 * 9) / (0.25 + 0.5 + 0.25)),
        ("weighted_sum", (0.25 * 7 + 0.5 * 8 + 0.25 * 9)),
        ("weighted_stdev", 0.7071068),
        ("weighted_variance", 0.5),
    ],
)
def test_weighted_stats_unequal_weights(stat, expected):
    rast = NumPyRasterSource(np.arange(1, 10).reshape(3, 3))
    weights = NumPyRasterSource(np.array([[0, 0, 0], [0, 0, 0], [1, 1, 1]]))
    square = JSONFeatureSource(make_rect(0.5, 0.5, 2.5, 2.5))

    assert exact_extract(rast, square, stat, weights=weights)[0]["properties"][
        stat
    ] == pytest.approx(expected)


def test_frac():
    rast = NumPyRasterSource(np.array([[1, 1, 1], [2, 2, 2], [3, 3, 3]]))
    squares = JSONFeatureSource(
        [make_rect(0.5, 0.5, 1.0, 1.0, "a"), make_rect(0.5, 0.5, 2.5, 2.5, "b")])

    results = exact_extract(rast, squares, ["count", "frac"])

    assert results[0]["properties"] == {
        "count": 0.25,
        # "frac_1" : 0,
        # "frac_2" : 0,
        "frac_3": 1.00,
    }

    assert results[1]["properties"] == {
        "count": 4,
        "frac_1": 0.25,
        "frac_2": 0.5,
        "frac_3": 0.25,
    }

    pytest.xfail("missing placeholder results for frac_1 and frac_2")


def test_weighted_frac():
    rast = NumPyRasterSource(np.array([[1, 1, 1], [2, 2, 2], [3, 3, 3]]))
    weights = NumPyRasterSource(np.array([[3, 3, 3], [2, 2, 2], [1, 1, 1]]))
    squares = JSONFeatureSource(
        [make_rect(0.5, 0.5, 1.0, 1.0, "a"), make_rect(0.5, 0.5, 2.5, 2.5, "b")])

    results = exact_extract(rast, squares, ["weighted_frac", "sum"], weights=weights)

    assert results[0]["properties"] == {
        # "weighted_frac_1" : 0,
        # "weighted_frac_2" : 0,
        "weighted_frac_3": 1,
        "sum": 0.75,
    }

    assert results[1]["properties"] == {
        "weighted_frac_1": 0.375,
        "weighted_frac_2": 0.5,
        "weighted_frac_3": 0.125,
        "sum": 8,
    }

    pytest.xfail("missing placeholder results for weighted_frac_1 and weighted_frac_2")



def create_gdal_raster(fname, values):
    gdal = pytest.importorskip("osgeo.gdal")
    drv = gdal.GetDriverByName("GTiff")

    ds = drv.Create(str(fname), values.shape[1], values.shape[0])
    ds.SetGeoTransform((0.0, 1.0, 0.0, values.shape[0], 0.0, -1.0))
    ds.WriteArray(values)


def create_gdal_features(fname, features, name="test"):
    gdal = pytest.importorskip("osgeo.gdal")

    import tempfile
    import json
    with tempfile.NamedTemporaryFile(suffix=".geojson") as tf:
        tf.write(json.dumps({
            "type":"FeatureCollection",
            "features": features
        }).encode())
        tf.flush()

        ds = gdal.VectorTranslate(str(fname), tf.name)
    

def test_gdal_inputs(tmp_path):
    gdal = pytest.importorskip("osgeo.gdal")
    ogr = pytest.importorskip("osgeo.ogr")

    raster_fname = str(tmp_path / "rast.tif")
    create_gdal_raster(raster_fname, np.arange(1, 10).reshape(3, 3))

    shp_fname = str(tmp_path / "geom.gpkg")
    squares = [
            make_rect(0.5, 0.5, 1.0, 1.0, "a"),
            make_rect(0.5, 0.5, 2.5, 2.5, "b")]

    create_gdal_features(shp_fname, squares)

    rast = gdal.Open(raster_fname)
    shp = ogr.Open(shp_fname)

    results = exact_extract(rast, shp, ["mean", "count"])

    assert len(results) == 2

    assert results[0]['properties'] == {
            'count' : 0.25, 
            'mean' : 7.0
    }

    assert results[1]['properties'] == {
            'count': 4.0,
            'mean': 5.0
    }


def test_fiona_inputs(tmp_path):
    fiona = pytest.importorskip("fiona")

    rast = NumPyRasterSource(np.arange(1, 10).reshape(3, 3))

    shp_fname = str(tmp_path / "geom.gpkg")
    squares = [
            make_rect(0.5, 0.5, 1.0, 1.0, "a"),
            make_rect(0.5, 0.5, 2.5, 2.5, "b")]
    create_gdal_features(shp_fname, squares)

    shp = fiona.open(shp_fname)

    results = exact_extract(rast, shp, ["mean", "count"])

    assert len(results) == 2

    assert results[0]['properties'] == {
            'count' : 0.25, 
            'mean' : 7.0
    }

    assert results[1]['properties'] == {
            'count': 4.0,
            'mean': 5.0
    }



def test_geopandas_inputs(tmp_path):
    gp = pytest.importorskip("geopandas")

    rast = NumPyRasterSource(np.arange(1, 10).reshape(3, 3))

    shp_fname = str(tmp_path / "geom.gpkg")
    squares = [
            make_rect(0.5, 0.5, 1.0, 1.0, "a"),
            make_rect(0.5, 0.5, 2.5, 2.5, "b")]
    create_gdal_features(shp_fname, squares)
    
    shp = gp.read_file(shp_fname)

    results = exact_extract(rast, shp, ["mean", "count"])

    assert len(results) == 2

    assert results[0]['properties'] == {
            'count' : 0.25, 
            'mean' : 7.0
    }

    assert results[1]['properties'] == {
            'count': 4.0,
            'mean': 5.0
    }


def test_rasterio_inputs(tmp_path):
    rasterio = pytest.importorskip("rasterio")

    raster_fname = str(tmp_path / "rast.tif")
    create_gdal_raster(raster_fname, np.arange(1, 10).reshape(3, 3))

    squares = [
            make_rect(0.5, 0.5, 1.0, 1.0, "a"),
            make_rect(0.5, 0.5, 2.5, 2.5, "b")]

    rast = rasterio.open(raster_fname)
    
    results = exact_extract(rast, squares, ["mean", "count"])

    assert len(results) == 2

    assert results[0]['properties'] == {
            'count' : 0.25, 
            'mean' : 7.0
    }

    assert results[1]['properties'] == {
            'count': 4.0,
            'mean': 5.0
    }


