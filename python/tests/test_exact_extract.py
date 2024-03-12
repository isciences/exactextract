import math
import os

import numpy as np
import pytest

from exactextract import exact_extract
from exactextract.feature import JSONFeatureSource
from exactextract.raster import NumPyRasterSource


@pytest.fixture()
def output_format(request):
    if request.param == "pandas":
        pytest.importorskip("pandas")

    return request.param


def prop_map(f, k, v):
    return {v: f for v, f in zip(f["properties"][k], f["properties"][v])}


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
        ("quantile(q=0.25)", {"quantile_25": 3}),
        ("quantile(q=0.75)", {"quantile_75": 6}),
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
        ("min_center_x", 0.5),
        ("min_center_y", 2.5),
        ("max_center_x", 2.5),
        ("max_center_y", 0.5),
        ("unique", {1, 2, 3, 4, 5, 6, 7, 8, 9}),
    ],
)
def test_basic_stats(stat, expected, output_format):
    rast = NumPyRasterSource(np.arange(1, 10, dtype=np.int32).reshape(3, 3))
    square = JSONFeatureSource(make_rect(0.5, 0.5, 2.5, 2.5))

    result = exact_extract(rast, square, stat, output=output_format)

    if type(expected) is dict:
        stat, expected = next(iter(expected.items()))

    if output_format == "geojson":
        value = result[0]["properties"][stat]
    else:
        value = result[stat][0]

    if type(expected) is set:
        assert set(value) == expected
        return

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

    results = exact_extract(rast, squares, ["count", "unique", "frac"])

    assert prop_map(results[0], "unique", "frac") == {3: 1.00}

    assert prop_map(results[1], "unique", "frac") == {1: 0.25, 2: 0.5, 3: 0.25}

    pytest.xfail("missing placeholder results for frac_1 and frac_2")


def test_weighted_frac():
    rast = NumPyRasterSource(np.array([[1, 1, 1], [2, 2, 2], [3, 3, 3]]))
    weights = NumPyRasterSource(np.array([[3, 3, 3], [2, 2, 2], [1, 1, 1]]))
    squares = JSONFeatureSource(
        [make_rect(0.5, 0.5, 1.0, 1.0, "a"), make_rect(0.5, 0.5, 2.5, 2.5, "b")]
    )

    results = exact_extract(
        rast, squares, ["weighted_frac", "sum", "unique"], weights=weights
    )

    assert results[0]["properties"]["sum"] == 0.75
    assert prop_map(results[0], "unique", "weighted_frac") == {
        3: 1,
    }

    assert results[1]["properties"]["sum"] == 8
    assert prop_map(results[1], "unique", "weighted_frac") == {
        1: 0.375,
        2: 0.5,
        3: 0.125,
    }

    pytest.xfail("missing placeholder results for weighted_frac_1 and weighted_frac_2")


def test_multiband():
    rast = [
        NumPyRasterSource(np.arange(1, 10).reshape(3, 3), name="a"),
        NumPyRasterSource(2 * np.arange(1, 10).reshape(3, 3), name="b"),
    ]

    squares = [make_rect(0.5, 0.5, 1.0, 1.0), make_rect(0.5, 0.5, 2.5, 2.5)]

    def py_mean(values, cov):
        return np.average(values, weights=cov)

    results = exact_extract(rast, squares, ["count", "mean", py_mean])

    assert len(results) == len(squares)

    assert results[0]["properties"] == {
        "a_mean": 7.0,
        "a_count": 0.25,
        "a_py_mean": 7.0,
        "b_mean": 14.0,
        "b_count": 0.25,
        "b_py_mean": 14.0,
    }

    assert results[1]["properties"] == {
        "a_mean": 5.0,
        "a_count": 4.0,
        "a_py_mean": 5.0,
        "b_mean": 10.0,
        "b_count": 4.0,
        "b_py_mean": 10.0,
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
            "a_mean": 7.0,
            "b_mean": 14.0,
            "a_w1_weighted_mean": 7.0,
            "b_w2_weighted_mean": float("nan"),
        },
        nan_ok=True,
    )

    assert results[1]["properties"] == {
        "a_mean": 5.0,
        "b_mean": 10.0,
        "a_w1_weighted_mean": 7.0,
        "b_w2_weighted_mean": 10.0,
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
        "a_mean": 5.0,
        "b_mean": 10.0,
        "a_w_weighted_mean": 7.0,
        "b_w_weighted_mean": 14.0,
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
        "a_mean": 5.0,
        "a_w1_weighted_mean": 7.0,
        "a_w2_weighted_mean": 5.0,
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
    elif libname == "qgis":
        qgis_core = pytest.importorskip("qgis.core")
        return qgis_core.QgsVectorLayer(fname)


@pytest.mark.parametrize("rast_lib", ("gdal", "rasterio", "xarray"))
@pytest.mark.parametrize("vec_lib", ("ogr", "fiona", "geopandas", "qgis"))
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
                    "band_1_count": 0.25,
                    "band_2_count": 0.25,
                    "band_1_mean": 7.0,
                    "band_2_mean": 14.0,
                },
                {
                    "band_1_count": 4.0,
                    "band_2_count": 4.0,
                    "band_1_mean": 5.0,
                    "band_2_mean": 10.0,
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

    # include_cols may also be a string
    results = exact_extract(rast, features, "count", include_cols="type")

    assert results[0]["properties"]["type"] == "apple"


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


@pytest.mark.parametrize(
    "output_type,driver",
    (("filename", None), ("filename", "GPKG"), ("DataSource", None)),
)
def test_gdal_output(tmp_path, output_type, driver):
    ogr = pytest.importorskip("osgeo.ogr")
    osr = pytest.importorskip("osgeo.osr")

    srs = osr.SpatialReference()
    srs.ImportFromEPSG(32145)

    rast = NumPyRasterSource(np.full((3, 3), 1), srs_wkt=srs.ExportToWkt())

    square = JSONFeatureSource(
        make_rect(0, 0, 3, 3, properties={"name": "test"}), srs_wkt=srs.ExportToWkt()
    )

    if output_type == "filename":
        fname = tmp_path / "stats.gpkg"
        output_options = {"filename": fname, "driver": driver}
    elif output_type == "DataSource":
        fname = tmp_path / "stats.dbf"
        drv = ogr.GetDriverByName("ESRI Shapefile")
        ds = drv.CreateDataSource(str(fname))
        output_options = {"dataset": ds}

    result = exact_extract(
        rast,
        square,
        ["mean", "count", "variety"],
        include_cols=["name"],
        output="gdal",
        output_options=output_options,
        include_geom=True,
    )

    assert result is None

    del output_options  # flush DataSource
    ds = None  # flush DataSource
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

    assert "Vermont" in lyr.GetSpatialRef().ExportToWkt()


def test_geopandas_output():
    gpd = pytest.importorskip("geopandas")
    osr = pytest.importorskip("osgeo.osr")

    srs = osr.SpatialReference()
    srs.ImportFromEPSG(32145)

    rast = NumPyRasterSource(
        np.arange(1, 10, dtype=np.int32).reshape(3, 3), srs_wkt=srs.ExportToWkt()
    )

    square = JSONFeatureSource(
        make_rect(0, 0, 3, 3, properties={"name": "test"}), srs_wkt=srs.ExportToWkt()
    )

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

    assert "Vermont" in str(result.geometry.crs)


def test_masked_array():
    data = np.arange(1, 10, dtype=np.int32).reshape(3, 3)
    invalid = np.array(
        [[False, False, False], [True, True, True], [False, False, False]]
    )

    masked = np.ma.masked_array(data, invalid)

    rast = NumPyRasterSource(masked)

    square = make_rect(0, 0, 3, 3)

    result = exact_extract(rast, square, ["values"])[0]["properties"]

    assert np.array_equal(result["values"], masked.compressed())


def test_output_no_numpy():

    rast = NumPyRasterSource(np.arange(1, 10, dtype=np.int32).reshape(3, 3))
    square = JSONFeatureSource(make_rect(0.5, 0.5, 2.5, 2.5))

    result = exact_extract(
        rast, square, "cell_id", output="geojson", output_options={"array_type": "list"}
    )

    assert type(result[0]["properties"]["cell_id"]) is list


def test_output_map_fields():

    rast = NumPyRasterSource(np.array([[1, 1, 1], [2, 2, 2], [3, 3, 4]]))
    weights = NumPyRasterSource(np.array([[0, 0, 0], [0, 0, 0], [1, 1, 1]]))
    square = make_rect(0.5, 0.5, 2.5, 2.5)

    result = exact_extract(
        rast,
        square,
        ["unique", "frac", "weighted_frac"],
        weights=weights,
        output_options={
            "map_fields": {
                "frac": ("unique", "frac"),
                "weighted_frac": ("unique", "weighted_frac"),
            }
        },
    )

    assert result[0]["properties"] == {
        "frac": {1: 0.25, 2: 0.5, 3: 0.1875, 4: 0.0625},
        "weighted_frac": {1: 0.0, 2: 0.0, 3: 0.75, 4: 0.25},
    }


def test_custom_function_throws_exception():

    rast = NumPyRasterSource(np.arange(9).reshape(3, 3))
    square = make_rect(0.5, 0.5, 2.5, 2.5)

    def stat_with_error(values, coverage):
        raise RuntimeError("errors are propagated")

    with pytest.raises(RuntimeError, match="errors are propagated"):
        exact_extract(rast, square, stat_with_error)


def test_custom_function_missing_weights():

    rast = NumPyRasterSource(np.arange(9).reshape(3, 3))
    square = make_rect(0.5, 0.5, 2.5, 2.5)

    def my_weighted_stat(values, coverage, weights):
        return 1

    with pytest.raises(Exception, match="No weights provided"):
        exact_extract(rast, square, my_weighted_stat)


def test_custom_function_bad_signature():

    rast = NumPyRasterSource(np.arange(9).reshape(3, 3))
    square = make_rect(0.5, 0.5, 2.5, 2.5)

    def no_args():
        return 1

    def one_arg(a):
        return 1

    def four_args(a, b, c, d):
        return 1

    with pytest.raises(Exception, match="must take 2 or 3 argument"):
        exact_extract(rast, square, no_args)

    with pytest.raises(Exception, match="must take 2 or 3 argument"):
        exact_extract(rast, square, one_arg)

    with pytest.raises(Exception, match="must take 2 or 3 argument"):
        exact_extract(rast, square, four_args)


def test_custom_function_bad_return_type():

    rast = NumPyRasterSource(np.arange(9).reshape(3, 3))
    square = make_rect(0.5, 0.5, 2.5, 2.5)

    def my_stat(x, c):
        return set()

    with pytest.raises(Exception, match="Unhandled type"):
        exact_extract(rast, square, my_stat)


@pytest.mark.parametrize(
    "ret",
    [1, 2.0, "cookie", None]
    + [
        np.arange(3, dtype=T)
        for T in (np.int8, np.int16, np.int32, np.int64, np.float32, np.float64)
    ],
    ids=lambda x: f"ndarray[{x.dtype}]" if hasattr(x, "dtype") else type(x),
)
def test_custom_function_return_types(ret):

    rast = NumPyRasterSource(np.arange(9).reshape(3, 3))
    square = make_rect(0.5, 0.5, 2.5, 2.5)

    def my_stat(values, coverage):
        return ret

    result = exact_extract(rast, square, my_stat)

    if isinstance(ret, np.ndarray):
        assert np.all(result[0]["properties"]["my_stat"] == ret)
    elif ret is None:
        pass
    else:
        assert result[0]["properties"]["my_stat"] == ret


def test_custom_function():

    rast = NumPyRasterSource(np.arange(9).reshape(3, 3))
    weights = NumPyRasterSource(np.sqrt(np.arange(9).reshape(3, 3)))
    square = make_rect(0.5, 0.5, 2.5, 2.5)

    def py_mean(values, coverage):
        return np.average(values, weights=coverage)

    def py_weighted_mean(values, coverage, weights):
        return np.average(values, weights=coverage * weights)

    result = exact_extract(
        rast,
        square,
        ["mean", py_mean, "weighted_mean", py_weighted_mean],
        weights=weights,
    )

    assert result[0]["properties"]["mean"] == pytest.approx(
        result[0]["properties"]["py_mean"]
    )
    assert result[0]["properties"]["weighted_mean"] == pytest.approx(
        result[0]["properties"]["py_weighted_mean"]
    )
