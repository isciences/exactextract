import math
import os

import numpy as np
import pytest

from exactextract import JSONFeatureSource, NumPyRasterSource, exact_extract


@pytest.fixture()
def output_format(request):
    if request.param == "pandas":
        pytest.importorskip("pandas")

    return request.param


def make_square_raster(n):
    values = np.arange(1, n * n + 1).reshape(n, n)
    return NumPyRasterSource(values, 0, 0, n, n)


def make_rect(xmin, ymin, xmax, ymax, id=None, properties=None):
    f = {
        "type": "Feature",
        "geometry": {
            "type": "Polygon",
            "coordinates": [
                [[xmin, ymin], [xmax, ymin], [xmax, ymax], [xmin, ymax], [xmin, ymin]]
            ],
        },
    }

    if id is not None:
        f["id"] = id
    if properties is not None:
        f["properties"] = properties

    return f


@pytest.mark.parametrize("output_format", ("geojson", "pandas"), indirect=True)
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
        ("values", np.array([1, 2, 3, 4, 5, 6, 7, 8, 9], dtype=np.int32)),
        (
            "coverage",
            np.array(
                [0.25, 0.5, 0.25, 0.5, 1.0, 0.5, 0.25, 0.5, 0.25], dtype=np.float64
            ),
        ),
        (
            "center_x",
            np.array([0.5, 1.5, 2.5, 0.5, 1.5, 2.5, 0.5, 1.5, 2.5], dtype=np.float64),
        ),
        (
            "center_y",
            np.array([2.5, 2.5, 2.5, 1.5, 1.5, 1.5, 0.5, 0.5, 0.5], dtype=np.float64),
        ),
        ("cell_id", np.array([0, 1, 2, 3, 4, 5, 6, 7, 8], dtype=np.int64)),
    ],
)
def test_basic_stats(stat, expected, output_format):
    rast = NumPyRasterSource(np.arange(1, 10, dtype=np.int32).reshape(3, 3))
    square = JSONFeatureSource(make_rect(0.5, 0.5, 2.5, 2.5))

    result = exact_extract(rast, square, stat, output=output_format)

    if output_format == "geojson":
        value = result[0]["properties"][stat]
    else:
        value = result[stat][0]

    assert value == pytest.approx(expected)

    if isinstance(expected, np.ndarray):
        assert expected.dtype == value.dtype


@pytest.mark.parametrize("output_format", ("geojson", "pandas"), indirect=True)
def test_multiple_stats(output_format):
    rast = NumPyRasterSource(np.arange(1, 10).reshape(3, 3))
    square = JSONFeatureSource(make_rect(0.5, 0.5, 2.5, 2.5))

    result = exact_extract(rast, square, ("min", "max", "mean"), output=output_format)

    if output_format == "geojson":
        fields = result[0]["properties"]
    else:
        fields = result.to_dict(orient="records")[0]

    assert fields == {
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
        ("weights", np.array([0, 0, 0, 0, 0, 0, 1, 1, 1])),
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
        [make_rect(0.5, 0.5, 1.0, 1.0, "a"), make_rect(0.5, 0.5, 2.5, 2.5, "b")]
    )

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
        [make_rect(0.5, 0.5, 1.0, 1.0, "a"), make_rect(0.5, 0.5, 2.5, 2.5, "b")]
    )

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


def test_multiband():
    rast = [
        NumPyRasterSource(np.arange(1, 10).reshape(3, 3), name="a"),
        NumPyRasterSource(2 * np.arange(1, 10).reshape(3, 3), name="b"),
    ]

    squares = [make_rect(0.5, 0.5, 1.0, 1.0), make_rect(0.5, 0.5, 2.5, 2.5)]

    results = exact_extract(rast, squares, ["count", "mean"])

    assert len(results) == len(squares)

    assert results[0]["properties"] == {
        "mean_a": 7.0,
        "count_a": 0.25,
        "mean_b": 14.0,
        "count_b": 0.25,
    }

    assert results[1]["properties"] == {
        "mean_a": 5.0,
        "count_a": 4.0,
        "mean_b": 10.0,
        "count_b": 4.0,
    }


def test_weighted_multiband():
    rast = [
        NumPyRasterSource(np.arange(1, 10).reshape(3, 3), name="a"),
        NumPyRasterSource(2 * np.arange(1, 10).reshape(3, 3), name="b"),
    ]

    weights = [
        NumPyRasterSource(np.array([[0, 0, 0], [0, 0, 0], [0.5, 0, 0]]), name="w1"),
        NumPyRasterSource(np.array([[0, 0, 0], [1, 1, 1], [0, 0, 0]]), name="w2"),
    ]

    squares = [make_rect(0.5, 0.5, 1.0, 1.0), make_rect(0.5, 0.5, 2.5, 2.5)]

    results = exact_extract(rast, squares, ["mean", "weighted_mean"], weights=weights)

    assert len(results) == len(squares)

    assert results[0]["properties"] == pytest.approx(
        {
            "mean_a": 7.0,
            "mean_b": 14.0,
            "weighted_mean_a_w1": 7.0,
            "weighted_mean_b_w2": float("nan"),
        },
        nan_ok=True,
    )

    assert results[1]["properties"] == {
        "mean_a": 5.0,
        "mean_b": 10.0,
        "weighted_mean_a_w1": 7.0,
        "weighted_mean_b_w2": 10.0,
    }


def test_weighted_multiband_values():
    rast = [
        NumPyRasterSource(np.arange(1, 10).reshape(3, 3), name="a"),
        NumPyRasterSource(2 * np.arange(1, 10).reshape(3, 3), name="b"),
    ]

    weights = NumPyRasterSource(np.array([[0, 0, 0], [0, 0, 0], [0.5, 0, 0]]), name="w")

    square = make_rect(0.5, 0.5, 2.5, 2.5)

    results = exact_extract(rast, square, ["mean", "weighted_mean"], weights=weights)

    assert len(results) == 1

    assert results[0]["properties"] == {
        "mean_a": 5.0,
        "mean_b": 10.0,
        "weighted_mean_a_w": 7.0,
        "weighted_mean_b_w": 14.0,
    }


def test_weighted_multiband_weights():
    rast = NumPyRasterSource(np.arange(1, 10).reshape(3, 3), name="a")

    weights = [
        NumPyRasterSource(np.array([[0, 0, 0], [0, 0, 0], [0.5, 0, 0]]), name="w1"),
        NumPyRasterSource(np.array([[0, 0, 0], [1, 1, 1], [0, 0, 0]]), name="w2"),
    ]

    square = make_rect(0.5, 0.5, 2.5, 2.5)

    results = exact_extract(rast, square, ["mean", "weighted_mean"], weights=weights)

    assert len(results) == 1

    assert results[0]["properties"] == {
        "mean_a": 5.0,
        "weighted_mean_a_w1": 7.0,
        "weighted_mean_a_w2": 5.0,
    }


def test_nodata():
    data = np.arange(1, 101).reshape(10, 10)
    data[6:10, 0:4] = -999  # flag lower-left corner as nodata
    rast = NumPyRasterSource(data, nodata=-999)

    # square entirely within nodata region
    square = make_rect(0, 0, 3, 3)
    results = exact_extract(rast, square, ["sum", "mean"])

    assert results[0]["properties"] == pytest.approx(
        {"sum": 0, "mean": float("nan")}, nan_ok=True
    )

    # square partially within nodata region
    square = make_rect(3.5, 3.5, 4.5, 4.5)

    results = exact_extract(rast, square, ["sum", "mean"])

    assert results[0]["properties"] == {"sum": 43.5, "mean": 58}


def create_gdal_raster(fname, values, *, gt=None):
    gdal = pytest.importorskip("osgeo.gdal")
    drv = gdal.GetDriverByName("GTiff")

    bands = 1 if len(values.shape) == 2 else values.shape[0]

    ds = drv.Create(str(fname), values.shape[-2], values.shape[-1], bands=bands)

    if gt is None:
        ds.SetGeoTransform((0.0, 1.0, 0.0, values.shape[-2], 0.0, -1.0))
    else:
        ds.SetGeoTransform(gt)

    if len(values.shape) == 2:
        ds.WriteArray(values)
    else:
        for i in range(bands):
            ds.GetRasterBand(i + 1).WriteArray(values[i, :, :])


def create_gdal_features(fname, features, name="test"):
    gdal = pytest.importorskip("osgeo.gdal")

    import json
    import tempfile

    # need to use delete=False so VectorTranslate can access the file on Windows
    with tempfile.NamedTemporaryFile(suffix=".geojson", delete=False) as tf:
        tf.write(
            json.dumps({"type": "FeatureCollection", "features": features}).encode()
        )
        tf.flush()

        ds = gdal.VectorTranslate(str(fname), tf.name)
        ds = None  # noqa: F841

    os.remove(tf.name)


def open_with_lib(fname, libname):
    if libname == "gdal":
        gdal = pytest.importorskip("osgeo.gdal")
        return gdal.Open(fname)
    elif libname == "rasterio":
        rasterio = pytest.importorskip("rasterio")
        return rasterio.open(fname)
    elif libname == "xarray":
        rioxarray = pytest.importorskip("rioxarray")  # noqa: F841
        xarray = pytest.importorskip("xarray")
        return xarray.open_dataarray(fname)
    elif libname == "ogr":
        ogr = pytest.importorskip("osgeo.ogr")
        return ogr.Open(fname)
    elif libname == "fiona":
        fiona = pytest.importorskip("fiona")
        return fiona.open(fname)
    elif libname == "geopandas":
        gp = pytest.importorskip("geopandas")
        return gp.read_file(fname)


@pytest.mark.parametrize("rast_lib", ("gdal", "rasterio", "xarray"))
@pytest.mark.parametrize("vec_lib", ("ogr", "fiona", "geopandas"))
@pytest.mark.parametrize(
    "arr,expected",
    [
        (
            np.arange(1, 10).reshape(3, 3),
            [{"count": 0.25, "mean": 7.0}, {"count": 4.0, "mean": 5.0}],
        ),
        (
            np.stack(
                [np.arange(1, 10).reshape(3, 3), 2 * np.arange(1, 10).reshape(3, 3)]
            ),
            [
                {
                    "count_band_1": 0.25,
                    "count_band_2": 0.25,
                    "mean_band_1": 7.0,
                    "mean_band_2": 14.0,
                },
                {
                    "count_band_1": 4.0,
                    "count_band_2": 4.0,
                    "mean_band_1": 5.0,
                    "mean_band_2": 10.0,
                },
            ],
        ),
    ],
    ids=["singleband", "multiband"],
)
def test_library_inputs(tmp_path, vec_lib, rast_lib, arr, expected):
    raster_fname = str(tmp_path / "rast.tif")
    create_gdal_raster(raster_fname, arr)

    shp_fname = str(tmp_path / "geom.gpkg")
    squares = [make_rect(0.5, 0.5, 1.0, 1.0, "a"), make_rect(0.5, 0.5, 2.5, 2.5, "b")]

    create_gdal_features(shp_fname, squares)

    rast = open_with_lib(raster_fname, rast_lib)
    shp = open_with_lib(shp_fname, vec_lib)

    results = exact_extract(rast, shp, ["mean", "count"])

    assert len(results) == len(expected)

    for a, e in zip(results, expected):
        assert a["properties"] == e


@pytest.mark.parametrize("strategy", ("feature-sequential", "raster-sequential"))
def test_include_cols(strategy):
    rast = NumPyRasterSource(np.arange(1, 10).reshape(3, 3))

    features = []
    features.append(
        make_rect(0.5, 0.5, 2.5, 2.5, id=1, properties={"type": "apple", "a": 4.1})
    )
    features.append(
        make_rect(0.5, 0.5, 2.5, 2.5, id=2, properties={"type": "pear", "a": 4.2})
    )

    results = exact_extract(
        rast, features, "count", include_cols=["a", "type", "id"], include_geom=True
    )

    assert results[0]["id"] == 1
    assert results[0]["properties"] == {"a": 4.1, "type": "apple", "count": 4.0}
    assert results[0]["geometry"]["type"] == "Polygon"
    assert results[0]["geometry"]["coordinates"][0][0] == [0.5, 0.5]

    assert results[1]["id"] == 2
    assert results[1]["properties"] == {"a": 4.2, "type": "pear", "count": 4.0}
    assert results[1]["geometry"]["type"] == "Polygon"


def test_error_no_weights():
    rast = NumPyRasterSource(np.arange(1, 13).reshape(3, 4))

    square = make_rect(0.5, 0.5, 2.5, 2.5)

    with pytest.raises(Exception, match="No weights provided"):
        exact_extract(rast, square, ["count", "weighted_mean"])


@pytest.mark.parametrize("rast_lib", ("gdal", "rasterio"))
def test_error_rotated_inputs(tmp_path, rast_lib):
    raster_fname = str(tmp_path / "rast.tif")

    create_gdal_raster(
        raster_fname,
        np.array([[1, 2], [3, 4]]),
        gt=(10000.0, 42.402, 26.496, 200000.0, 26.496, -42.402),
    )

    square = make_rect(10150, 199872, 10224, 199950)

    rast = open_with_lib(raster_fname, rast_lib)

    with pytest.raises(ValueError, match="Rotated raster"):
        exact_extract(rast, square, ["count"])


@pytest.mark.parametrize("dtype", (np.float64, np.float32, np.int32, np.int64))
def test_types_preserved(dtype):

    rast = NumPyRasterSource(np.full((3, 3), 1, dtype))

    square = make_rect(0, 0, 3, 3)

    result = exact_extract(rast, square, "mode")[0]["properties"]["mode"]

    if np.issubdtype(dtype, np.integer):
        assert isinstance(result, int)
    else:
        assert isinstance(result, float)


@pytest.mark.parametrize("output_type", ("filename", "DataSource"))
def test_gdal_output(tmp_path, output_type):
    ogr = pytest.importorskip("osgeo.ogr")

    rast = NumPyRasterSource(np.full((3, 3), 1))

    square = make_rect(0, 0, 3, 3, properties={"name": "test"})

    fname = tmp_path / "stats.dbf"

    if output_type == "filename":
        output = fname
    elif output_type == "DataSource":
        drv = ogr.GetDriverByName("ESRI Shapefile")
        output = drv.CreateDataSource(str(fname))

    result = exact_extract(
        rast,
        square,
        ["mean", "count", "variety"],
        include_cols=["name"],
        output=output,
        include_geom=True,
    )

    assert result is None

    output = None  # flush DataSource
    ds = ogr.Open(str(fname))

    lyr = ds.GetLayer(0)

    assert lyr.GetFeatureCount() == 1

    defn = lyr.GetLayerDefn()

    assert defn.GetFieldCount() == 4

    assert defn.GetFieldDefn(0).GetName() == "name"
    assert defn.GetFieldDefn(0).GetTypeName() == "String"

    assert defn.GetFieldDefn(1).GetName() == "mean"
    assert defn.GetFieldDefn(1).GetTypeName() == "Real"

    assert defn.GetFieldDefn(2).GetName() == "count"
    assert defn.GetFieldDefn(2).GetTypeName() == "Real"

    assert defn.GetFieldDefn(3).GetName() == "variety"
    assert defn.GetFieldDefn(3).GetTypeName() == "Integer"

    f = lyr.GetNextFeature()

    assert f["name"] == "test"
    assert f["mean"] == 1.0
    assert f["count"] == 9.0
    assert f["variety"] == 1
    assert f.GetGeometryRef().GetArea() == 9.0


def test_geopandas_output():
    gpd = pytest.importorskip("geopandas")

    rast = NumPyRasterSource(np.arange(1, 10, dtype=np.int32).reshape(3, 3))

    square = make_rect(0, 0, 3, 3, properties={"name": "test"})

    result = exact_extract(
        rast,
        square,
        ["mean", "count", "variety"],
        include_cols=["name"],
        output="pandas",
        include_geom=True,
    )

    assert isinstance(result, gpd.GeoDataFrame)
    assert result.area[0] == 9
