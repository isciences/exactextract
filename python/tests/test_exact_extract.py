import contextlib
import math
import os
import warnings

import numpy as np
import pytest

from exactextract import Operation, exact_extract
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


@contextlib.contextmanager
def use_gdal_exceptions():
    from osgeo import gdal

    prev = gdal.GetUseExceptions()
    gdal.UseExceptions()
    yield
    if not prev:
        gdal.UseExceptions()


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


def test_coverage_ignore_fraction():
    # when coverage_weight=none, we ignore the coverage fraction and count
    # all touched pixels equally
    rast = NumPyRasterSource(np.arange(1, 10, dtype=np.int32).reshape(3, 3))
    square = JSONFeatureSource(make_rect(0.5, 1.0, 2.5, 2.5))

    result = exact_extract(
        rast,
        square,
        ["mean(coverage_weight=none)", "count(coverage_weight=none)"],
    )[0]["properties"]

    assert result == {"count": 6.0, "mean": 3.5}


def test_min_coverage():
    # with min_coverage_frac we can exclude pixels that are not fully covered
    rast = NumPyRasterSource(np.arange(1, 10, dtype=np.int32).reshape(3, 3))
    square = JSONFeatureSource(make_rect(0.5, 0.5, 2.5, 2.5))

    result = exact_extract(
        rast,
        square,
        ["cell_id(min_coverage_frac=0.49)", "count(min_coverage_frac=0.49)"],
    )[0]["properties"]

    assert list(result["cell_id"]) == [1, 3, 4, 5, 7]
    assert result["count"] == 3.0


def test_min_coverage_ignore_fraction():
    rast = NumPyRasterSource(np.arange(1, 10, dtype=np.int32).reshape(3, 3))
    square = JSONFeatureSource(make_rect(0.5, 0.5, 2.5, 2.5))

    result = exact_extract(
        rast,
        square,
        [
            "cell_id(min_coverage_frac=0,coverage_weight=none)",
            "sum(min_coverage_frac=0,coverage_weight=none)",
        ],
    )[0]["properties"]

    assert list(result["cell_id"]) == [0, 1, 2, 3, 4, 5, 6, 7, 8]
    assert result["sum"] == np.arange(1, 10).sum()


@pytest.mark.parametrize("strategy", ("feature-sequential", "raster-sequential"))
def test_coverage_area(strategy):
    rast = NumPyRasterSource(np.arange(1, 10, dtype=np.int32).reshape(3, 3))

    square = JSONFeatureSource(make_rect(0.5, 0.5, 2.5, 2.5))

    result = exact_extract(
        rast,
        square,
        [
            "m1=mean",
            "m2=mean(coverage_weight=area_spherical_m2)",
            "c1=coverage",
            "c2=coverage(coverage_weight=area_spherical_m2)",
            "c3=coverage(coverage_weight=area_spherical_km2)",
            "c4=coverage(coverage_weight=area_cartesian)",
        ],
        strategy=strategy,
    )[0]["properties"]

    assert result["m2"] > result["m1"]

    assert np.all(np.isclose(result["c3"], result["c2"] * 1e-6))
    assert np.all(result["c4"] == result["c1"])


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


def test_multisource():
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


def test_weighted_multisource():
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


def test_multifile(tmp_path):
    rast1 = tmp_path / "rast1.tif"
    rast2 = tmp_path / "rast2.tif"

    create_gdal_raster(rast1, np.arange(1, 10, dtype=np.int32).reshape(3, 3))
    create_gdal_raster(rast2, 2 * np.arange(1, 10, dtype=np.int32).reshape(3, 3))

    square = make_rect(0.5, 0.5, 2.5, 2.5)

    results = exact_extract([rast1, rast2], square, "mean")

    assert results[0]["properties"] == {"rast1_mean": 5.0, "rast2_mean": 10.0}


def test_multiband(tmp_path):
    rast1 = tmp_path / "rast1.tif"
    rast2 = tmp_path / "rast2.tif"

    create_gdal_raster(
        rast1,
        np.stack(
            [np.arange(1, 10).reshape(3, 3), 2 * np.arange(1, 10).reshape(3, 3)]
        ).astype(np.int32),
    )

    create_gdal_raster(
        rast2,
        np.stack(
            [3 * np.arange(1, 10).reshape(3, 3), 4 * np.arange(1, 10).reshape(3, 3)]
        ).astype(np.int32),
    )

    square = make_rect(0.5, 0.5, 2.5, 2.5)

    results = exact_extract(rast1, square, "mean")

    assert results[0]["properties"] == {"band_1_mean": 5.0, "band_2_mean": 10.0}

    results = exact_extract([rast1, rast2], square, "mean")

    assert results[0]["properties"] == {
        "rast1_band_1_mean": 5.0,
        "rast1_band_2_mean": 10.0,
        "rast2_band_1_mean": 15.0,
        "rast2_band_2_mean": 20.0,
    }

    results = exact_extract(rast1, square, "weighted_mean", weights=rast2)

    assert sorted(results[0]["properties"].keys()) == [
        "band_1_weight_band_1_weighted_mean",
        "band_2_weight_band_2_weighted_mean",
    ]


def test_weighted_multisource_values():
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


def test_weighted_multisource_weights():
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


@pytest.mark.parametrize("libname", ("gdal", "rasterio", "xarray"))
def test_nodata_scale_offset(tmp_path, libname):
    gdal = pytest.importorskip("osgeo.gdal")

    data = np.array([[1, 2, 3], [-999, -999, -999], [4, 5, -499]])

    raster_fname = tmp_path / "test.tif"

    create_gdal_raster(
        raster_fname, data, gdal_type=gdal.GDT_Int16, nodata=-999, scale=2, offset=-1
    )

    square = make_rect(0.5, 0.5, 2.5, 2.5)

    rast = open_with_lib(raster_fname, libname)
    results = exact_extract(rast, square, "values")

    values = results[0]["properties"]["values"]

    assert values.dtype == np.float64
    np.testing.assert_array_equal(values, [1, 3, 5, 7, 9, -999])


@pytest.mark.parametrize("libname", ("gdal", "rasterio", "xarray"))
def test_gdal_mask_band(tmp_path, libname):
    gdal = pytest.importorskip("osgeo.gdal")

    data = np.array([[1, 2, 3], [0, 0, 0], [4, 5, 6]])
    data = np.ma.masked_array(data, data == 0)

    raster_fname = tmp_path / "test.tif"

    create_gdal_raster(raster_fname, data, gdal_type=gdal.GDT_Int16)

    square = make_rect(0.5, 0.5, 2.5, 2.5)

    rast = open_with_lib(raster_fname, libname)
    results = exact_extract(rast, square, "values")

    values = results[0]["properties"]["values"]

    np.testing.assert_array_equal(values, [1, 2, 3, 4, 5, 6])


def test_all_nodata_geojson():
    data = np.full((3, 3), -999, dtype=np.int32)
    rast = NumPyRasterSource(data, nodata=-999)

    square = make_rect(0.5, 0.5, 2.5, 2.5)
    results = exact_extract(rast, square, ["mean", "mode", "variety"], output="geojson")

    props = results[0]["properties"]

    assert math.isnan(props["mean"])
    assert props["variety"] == 0
    assert props["mode"] is None


def test_all_nodata_pandas():
    pytest.importorskip("pandas")

    data = np.full((3, 3), -999, dtype=np.int32)
    rast = NumPyRasterSource(data, nodata=-999)

    square = make_rect(0.5, 0.5, 2.5, 2.5)
    results = exact_extract(rast, square, ["mean", "mode", "variety"], output="pandas")

    assert math.isnan(results["mean"][0])
    assert results["variety"][0] == 0
    assert results["mode"][0] is None


def test_all_nodata_qgis():

    pytest.importorskip("qgis.core")

    data = np.full((3, 3), -999, dtype=np.int32)
    rast = NumPyRasterSource(data, nodata=-999)

    square = make_rect(0.5, 0.5, 2.5, 2.5)
    results = exact_extract(
        rast, square, ["mean", "mode", "variety"], output="qgis", include_geom=True
    )

    props = next(results.getFeatures()).attributeMap()

    assert math.isnan(props["mean"])
    assert props["variety"] == 0
    assert props["mode"] is None


def test_all_nodata_gdal():

    ogr = pytest.importorskip("osgeo.ogr")

    data = np.array([[1, 1, 1], [-999, -999, -999], [-999, -999, -999]], dtype=np.int32)
    rast = NumPyRasterSource(data, nodata=-999)

    ds = ogr.GetDriverByName("Memory").CreateDataSource("")

    squares = [make_rect(0.5, 0.5, 2.5, 2.5), make_rect(0.5, 0.5, 1.5, 1.5)]
    exact_extract(
        rast,
        squares,
        ["mean", "mode", "variety"],
        output="gdal",
        include_geom=True,
        output_options={"dataset": ds},
    )

    features = [f for f in ds.GetLayer(0)]

    assert math.isnan(features[1]["mean"])
    assert features[1]["variety"] == 0
    assert features[1]["mode"] is None


def test_default_value():
    data = np.array([[1, 2, 3], [4, -99, -99], [-99, 8, 9]])
    rast = NumPyRasterSource(data, nodata=-99)

    square = make_rect(0, 0, 3, 3)

    results = exact_extract(
        rast,
        square,
        [
            "count",
            "count_with_default=count(default_value=0)",
            "sum",
            "sum_with_default=sum(default_value=123)",
            "mean",
            "mean_with_default=mean(default_value=123)",
        ],
    )

    masked = np.ma.masked_array(data, data == -99)
    substituted = np.where(data == -99, 123, data)

    assert results[0]["properties"] == {
        "count": (~masked.mask).sum(),
        "count_with_default": substituted.size,
        "sum": masked.sum(),
        "sum_with_default": substituted.sum(),
        "mean": masked.mean(),
        "mean_with_default": substituted.mean(),
    }


def test_default_value_out_of_range_int():
    data = np.array([[1, 2, 3], [4, -99, -99], [-99, 8, 9]], dtype=np.int8)
    rast = NumPyRasterSource(data, nodata=-99)

    square = make_rect(0, 0, 3, 3)

    exact_extract(rast, square, "sum(default_value=127)")

    with pytest.raises(RuntimeError, match="out of range"):
        exact_extract(rast, square, "sum(default_value=128)")


def test_default_weight():
    values = NumPyRasterSource(np.full(9, 1).reshape(3, 3))
    weights = NumPyRasterSource(
        np.ma.masked_array(
            np.full(9, 1).reshape(3, 3),
            np.array(
                [[False, False, False], [False, False, False], [False, False, True]]
            ),
        )
    )

    square = make_rect(0, 0, 3, 3)

    results = exact_extract(
        values,
        square,
        ["w1=weighted_mean", "w2=weighted_mean(default_weight=0)"],
        weights=weights,
    )

    assert results[0]["properties"] == pytest.approx(
        {"w1": float("nan"), "w2": 1.0}, nan_ok=True
    )


def create_gdal_raster(
    fname,
    values,
    *,
    gt=None,
    gdal_type=None,
    nodata=None,
    scale=None,
    offset=None,
    crs=None,
):
    gdal = pytest.importorskip("osgeo.gdal")
    gdal_array = pytest.importorskip("osgeo.gdal_array")
    osr = pytest.importorskip("osgeo.osr")

    drv = gdal.GetDriverByName("GTiff")

    bands = 1 if len(values.shape) == 2 else values.shape[0]

    if gdal_type is None:
        gdal_type = gdal_array.NumericTypeCodeToGDALTypeCode(values.dtype)

    ds = drv.Create(
        str(fname), values.shape[-2], values.shape[-1], bands=bands, eType=gdal_type
    )

    if gt is None:
        ds.SetGeoTransform((0.0, 1.0, 0.0, values.shape[-2], 0.0, -1.0))
    else:
        ds.SetGeoTransform(gt)

    if crs is not None:
        srs = osr.SpatialReference()
        if crs.startswith("EPSG"):
            srs.ImportFromEPSG(int(crs.strip("EPSG:")))
        else:
            srs.ImportFromWkt(crs)
        ds.SetSpatialRef(srs)

    if nodata is not None:
        if type(nodata) in {list, tuple}:
            for i, v in enumerate(nodata):
                ds.GetRasterBand(i + 1).SetNoDataValue(v)
        else:
            ds.GetRasterBand(1).SetNoDataValue(nodata)

    if scale is not None:
        if type(scale) in {list, tuple}:
            for i, v in enumerate(scale):
                ds.GetRasterBand(i + 1).SetScale(v)
        else:
            ds.GetRasterBand(1).SetScale(scale)

    if offset is not None:
        if type(offset) in {list, tuple}:
            for i, v in enumerate(offset):
                ds.GetRasterBand(i + 1).SetOffset(v)
        else:
            ds.GetRasterBand(1).SetOffset(offset)

    if len(values.shape) == 2:
        ds.WriteArray(values)
    else:
        for i in range(bands):
            ds.GetRasterBand(i + 1).WriteArray(values[i, :, :])

    if hasattr(values, "mask"):
        ds.GetRasterBand(1).CreateMaskBand(gdal.GMF_PER_DATASET)
        ds.GetRasterBand(1).GetMaskBand().WriteArray(~values.mask)


def create_gdal_features(fname, features, name="test", *, crs=None):
    gdal = pytest.importorskip("osgeo.gdal")

    import json
    import tempfile

    # need to use delete=False so VectorTranslate can access the file on Windows
    with tempfile.NamedTemporaryFile(suffix=".geojson", delete=False) as tf:
        tf.write(
            json.dumps({"type": "FeatureCollection", "features": features}).encode()
        )
        tf.flush()

        with use_gdal_exceptions():
            ds = gdal.VectorTranslate(str(fname), tf.name, dstSRS=crs)
            ds = None  # noqa: F841

    os.remove(tf.name)


def open_with_lib(fname, libname):
    if libname == "gdal":
        gdal = pytest.importorskip("osgeo.gdal")
        return gdal.Open(str(fname))
    elif libname == "rasterio":
        rasterio = pytest.importorskip("rasterio")
        return rasterio.open(fname)
    elif libname == "xarray":
        rioxarray = pytest.importorskip("rioxarray")  # noqa: F841
        xarray = pytest.importorskip("xarray")
        try:
            return xarray.open_dataarray(fname)
        except ValueError:
            return xarray.open_dataset(fname)
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
            np.arange(1, 10, dtype=np.float32).reshape(3, 3),
            [{"count": 0.25, "mean": 7.0}, {"count": 4.0, "mean": 5.0}],
        ),
        (
            np.stack(
                [
                    np.arange(1, 10, dtype=np.float32).reshape(3, 3),
                    2 * np.arange(1, 10).reshape(3, 3),
                ]
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


@pytest.mark.parametrize(
    "dtype",
    (
        "Byte",
        "Int8",
        "Int16",
        "UInt16",
        "Int32",
        "UInt32",
        "Int64",
        "UInt64",
        "Float32",
        "Float64",
    ),
)
@pytest.mark.parametrize("rast_lib", ("gdal", "rasterio"))
def test_gdal_data_types(tmp_path, rast_lib, dtype):
    gdal = pytest.importorskip("osgeo.gdal")

    try:
        gdal_type = gdal.__dict__[f"GDT_{dtype}"]
    except KeyError:
        pytest.skip(f"GDAL data type {dtype} not available")

    raster_fname = str(tmp_path / "test.tif")

    create_gdal_raster(
        raster_fname,
        np.array([[1, 1, 1], [2, 2, 2], [3, 2, 3]]),
        gdal_type=gdal_type,
        nodata=2,
    )

    rast = open_with_lib(raster_fname, rast_lib)

    square = make_rect(0.5, 0.5, 2.5, 2.5)

    results = exact_extract(rast, square, "mode")[0]["properties"]

    assert results["mode"] == 1
    if "Float" in dtype:
        assert type(results["mode"]) is float
    else:
        assert type(results["mode"]) is int


# Check that a negative nodata value for a raster with type GDT_Byte is ignored rather than causing an exception
def test_gdal_invalid_nodata_value(tmp_path):
    gdal = pytest.importorskip("osgeo.gdal")

    raster_fname = str(tmp_path / "test.tif")

    create_gdal_raster(
        raster_fname,
        np.array([[1, 1, 1], [2, 2, 2], [3, 2, 3]]),
        gdal_type=gdal.GDT_Byte,
        nodata=-2,
    )

    square = make_rect(0.5, 0.5, 2.5, 2.5)

    results = exact_extract(raster_fname, square, "mode")[0]["properties"]

    assert results["mode"] == 2


@pytest.mark.parametrize("dtype", (np.uint8, np.uint16, np.uint32, np.uint64))
def test_unsigned_values_preserved(dtype):
    max_val = np.iinfo(dtype).max

    rast = NumPyRasterSource(
        np.array([[max_val, max_val], [max_val - 1, max_val - 1]], dtype=dtype),
        nodata=max_val - 1,
    )

    square = make_rect(0, 0, 2, 2)

    stats = ["sum"]
    if dtype != np.uint64:
        stats.append("mode")  # large uint64 cannot be stored as a feature attribute

    results = exact_extract(rast, square, stats)[0]["properties"]

    if dtype == np.uint64:
        # sum is processed as a double, max uint64 cannot be represented exactly
        assert results["sum"] == pytest.approx(2 * max_val)
    else:
        assert results["sum"] == 2 * max_val
        assert results["mode"] == max_val


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
    results = exact_extract(
        rast, features, "count", include_cols="type", strategy=strategy
    )

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
        np.array([[1, 2], [3, 4]], dtype=np.int16),
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


def test_qgis_output():
    qgis_core = pytest.importorskip("qgis.core")

    srs = qgis_core.QgsCoordinateReferenceSystem(3120)

    rast = NumPyRasterSource(
        np.arange(1, 10, dtype=np.int32).reshape(3, 3), srs_wkt=srs.toWkt()
    )

    square = JSONFeatureSource(
        make_rect(0, 0, 3, 3, properties={"name": "test"}), srs_wkt=srs.toWkt()
    )

    result = exact_extract(
        rast,
        square,
        ["mean", "count", "variety"],
        include_cols=["name"],
        output="qgis",
        include_geom=True,
    )

    features = [f for f in result.getFeatures()]

    assert len(features) == 1

    assert features[0].attributeMap() == {
        "count": 9.0,
        "mean": 5.0,
        "name": "test",
        "variety": 9,
    }

    assert (
        features[0].geometry().asWkt().upper() == "POLYGON ((0 0, 3 0, 3 3, 0 3, 0 0))"
    )


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
        "frac(min_coverage_frac=0.5)",
        output_options={"frac_as_map": True},
    )

    assert result[0]["properties"] == {
        "frac": {1: 0.5 / 3, 2: 2 / 3, 3: 0.5 / 3},
    }

    result = exact_extract(
        rast,
        square,
        ["frac", "weighted_frac", "unique"],
        weights=weights,
        output_options={"frac_as_map": True, "array_type": "set"},
    )

    assert result[0]["properties"] == {
        "frac": {1: 0.25, 2: 0.5, 3: 0.1875, 4: 0.0625},
        "weighted_frac": {1: 0.0, 2: 0.0, 3: 0.75, 4: 0.25},
        "unique": {1, 2, 3, 4},
    }


def test_output_map_fields_multiband():

    rast = [
        NumPyRasterSource(np.arange(1, 10).reshape(3, 3), name="landcov"),
        NumPyRasterSource(2 * np.arange(1, 10).reshape(3, 3), name="landcat"),
    ]

    weights = [
        NumPyRasterSource(np.array([[0, 0, 0], [0, 0, 0], [0.5, 0, 0]]), name="w1"),
        NumPyRasterSource(np.array([[0, 0, 0], [1, 1, 1], [0, 0, 0]]), name="w2"),
    ]

    squares = [make_rect(0.5, 0.5, 1.0, 1.0), make_rect(0.5, 0.5, 2.5, 2.5)]

    result = exact_extract(
        rast,
        squares,
        ["frac", "weighted_frac"],
        weights=weights,
        output_options={"frac_as_map": True},
    )

    assert set(result[1]["properties"].keys()) == {
        "landcat_frac",
        "landcov_frac",
        "landcat_w2_weighted_frac",
        "landcov_w1_weighted_frac",
    }


def test_linear_geom():

    rast = NumPyRasterSource(np.arange(1, 10).reshape(3, 3))
    line = {
        "type": "Feature",
        "geometry": {
            "type": "LineString",
            "coordinates": [[0.5, 0.5], [1.5, 1.5], [2.5, 0.5]],
        },
    }

    def frac_below_6(value, length):
        return np.sum(length * (value < 6)) / np.sum(length)

    results = exact_extract(rast, line, ["count", "mean", frac_below_6])

    assert results[0]["properties"] == pytest.approx(
        {
            "count": 2 * math.sqrt(2),  # length of line
            "mean": 0.5 * 5 + 0.25 * 7 + 0.25 * 9,  # length-weighted average
            "frac_below_6": 0.5,  # custom function
        }
    )


def test_point_geom():

    rast = NumPyRasterSource(np.arange(1, 10).reshape(3, 3))

    point = {
        "type": "Feature",
        "geometry": {"type": "Point", "coordinates": [1.5, 1.5]},
    }

    with pytest.raises(Exception, match="Unsupported geometry type"):
        exact_extract(rast, point, "mean")


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


@pytest.mark.parametrize("weighted", (False, True))
def test_custom_function_nodata(weighted):
    rast = NumPyRasterSource(np.arange(9).reshape(3, 3), nodata=7)
    rast_weights = NumPyRasterSource(np.arange(9).reshape(3, 3), nodata=6)
    square = make_rect(0.5, 0.5, 2.5, 2.5)

    values = None
    weights = None
    coverage = None

    def py_get_values(v, c):
        py_get_weighted_values(v, c, None)

    def py_get_weighted_values(v, c, w):
        nonlocal values, coverage, weights

        values = v
        coverage = c

        if w is not None:
            weights = w

    if weighted:
        results = exact_extract(
            rast,
            square,
            [py_get_weighted_values, "values", "coverage"],
            weights=rast_weights,
        )

        # Unlike values, weights are always coerced to doubles, because they are considered
        # to have no meaning except when multiplied by the coverage fraction (itself a double)
        assert len(weights) == 9
        np.testing.assert_array_equal(
            weights.data, [0, 1, 2, 3, 4, 5, float("nan"), 7, 8]
        )
        np.testing.assert_array_equal(
            weights.mask, [False, False, False, False, False, False, True, False, False]
        )

    else:
        results = exact_extract(rast, square, [py_get_values, "values", "coverage"])

    # The NODATA value of 7 comes out, but the value of the masked array
    # indicates that it should be considered nodata.
    assert len(values) == 9
    np.testing.assert_array_equal(values.data, [0, 1, 2, 3, 4, 5, 6, 7, 8])
    np.testing.assert_array_equal(
        values.mask, [False, False, False, False, False, False, False, True, False]
    )

    assert len(coverage) == 9
    np.testing.assert_array_equal(
        coverage, [0.25, 0.5, 0.25, 0.5, 1, 0.5, 0.25, 0.5, 0.25]
    )

    # The built-in operations do not consider NODATA values, despite being invoked at
    # the same time as a Python operation that does
    np.testing.assert_array_equal(
        results[0]["properties"]["values"], [0, 1, 2, 3, 4, 5, 6, 8]
    )
    np.testing.assert_array_equal(
        results[0]["properties"]["coverage"], [0.25, 0.5, 0.25, 0.5, 1, 0.5, 0.25, 0.25]
    )


def test_custom_function_not_intersecting():
    rast = NumPyRasterSource(np.arange(9).reshape(3, 3))
    faraway_square = make_rect(100, 100, 200, 200)

    def py_sentinel(v, c):
        pytest.fail("function was called")

    def py_sentinel_weighted(v, c, w):
        pytest.fail("function was called")

    assert exact_extract(rast, faraway_square, py_sentinel)[0]["properties"] == {
        "py_sentinel": None
    }
    assert exact_extract(rast, faraway_square, py_sentinel_weighted, weights=rast)[0][
        "properties"
    ] == {"py_sentinel_weighted": None}


def test_explicit_operation():

    rast = NumPyRasterSource(np.arange(9).reshape(3, 3))
    square = make_rect(0.5, 0.5, 2.5, 2.5)

    op = Operation("mean", "my_op", rast)

    results = exact_extract(rast, square, ["mean", op])

    assert results[0]["properties"]["mean"] == results[0]["properties"]["my_op"]

    results = exact_extract(None, square, op)

    assert results[0]["properties"]["my_op"] == 4.0


def test_progress():

    rast = NumPyRasterSource(np.arange(9).reshape(3, 3))
    square = make_rect(0.5, 0.5, 2.5, 2.5)

    squares = [square] * 10

    exact_extract(rast, squares, "count", progress=True)


def test_grid_compat_tol():

    values = NumPyRasterSource(
        np.arange(9).reshape(3, 3), xmin=0, xmax=1, ymin=0, ymax=1
    )
    weights = NumPyRasterSource(
        np.arange(9).reshape(3, 3), xmin=1e-3, xmax=1 + 1e-3, ymin=0, ymax=1
    )

    square = make_rect(0.1, 0.1, 0.9, 0.9)

    with pytest.raises(RuntimeError, match="Incompatible extents"):
        exact_extract(values, square, "weighted_mean", weights=weights)

    exact_extract(
        values, square, "weighted_mean", weights=weights, grid_compat_tol=1e-2
    )


def test_crs_mismatch(tmp_path):
    crs_list = (4269, 4326, None)

    rasters = {}
    features = {}
    for crs in crs_list:
        rasters[crs] = tmp_path / f"{crs}.tif"
        features[crs] = tmp_path / f"{crs}.shp"
        create_gdal_raster(
            rasters[crs],
            np.arange(9, dtype=np.int32).reshape(3, 3),
            crs=f"EPSG:{crs}" if crs else None,
        )
        create_gdal_features(
            features[crs],
            [make_rect(0.5, 0.5, 2.5, 2.5)],
            crs=f"EPSG:{crs}" if crs else None,
        )

    with pytest.warns(
        RuntimeWarning, match="input features does not exactly match raster"
    ) as record:
        exact_extract(rasters[4326], features[4269], "mean")
    assert len(record) == 1

    with pytest.warns(
        RuntimeWarning, match="input features does not exactly match raster"
    ) as record:
        exact_extract([rasters[4326], rasters[4269]], features[4326], "mean")
    assert len(record) == 1

    with pytest.warns(
        RuntimeWarning, match="input features does not exactly match weighting raster"
    ) as record:
        exact_extract(
            rasters[4326], features[4326], "weighted_mean", weights=rasters[4269]
        )
    assert len(record) == 1

    # make sure only a single warning is raised
    with pytest.warns(
        RuntimeWarning, match="input features does not exactly match raster"
    ) as record:
        exact_extract([rasters[4326], rasters[4269]], features[4326], ["mean", "sum"])
    assert len(record) == 1

    # any CRS is considered to match an undefined CRS
    with warnings.catch_warnings():
        warnings.simplefilter("error")
        exact_extract(rasters[None], features[4326], "mean")


def test_crs_match_after_normalization(tmp_path):

    pytest.importorskip("osgeo.osr")

    rast = tmp_path / "test.tif"
    square = tmp_path / "test.shp"

    rast_crs = 'PROJCS["WGS 84 / UTM zone 4N",GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.0174532925199433,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4326"]],PROJECTION["Transverse_Mercator"],PARAMETER["latitude_of_origin",0],PARAMETER["central_meridian",-159],PARAMETER["scale_factor",0.9996],PARAMETER["false_easting",500000],PARAMETER["false_northing",0],UNIT["metre",1,AUTHORITY["EPSG","9001"]],AXIS["Easting",EAST],AXIS["Northing",NORTH],AUTHORITY["EPSG","32604"]]'

    vec_crs = 'PROJCRS["WGS 84 / UTM zone 4N",BASEGEOGCRS["WGS 84",DATUM["World Geodetic System 1984",ELLIPSOID["WGS 84",6378137,298.257223563,LENGTHUNIT["metre",1]]],PRIMEM["Greenwich",0,ANGLEUNIT["degree",0.0174532925199433]],ID["EPSG",4326]],CONVERSION["UTM zone 4N",METHOD["Transverse Mercator",ID["EPSG",9807]],PARAMETER["Latitude of natural origin",0,ANGLEUNIT["degree",0.0174532925199433],ID["EPSG",8801]],PARAMETER["Longitude of natural origin",-159,ANGLEUNIT["degree",0.0174532925199433],ID["EPSG",8802]],PARAMETER["Scale factor at natural origin",0.9996,SCALEUNIT["unity",1],ID["EPSG",8805]],PARAMETER["False easting",500000,LENGTHUNIT["metre",1],ID["EPSG",8806]],PARAMETER["False northing",0,LENGTHUNIT["metre",1],ID["EPSG",8807]]],CS[Cartesian,2],AXIS["easting",east,ORDER[1],LENGTHUNIT["metre",1]],AXIS["northing",north,ORDER[2],LENGTHUNIT["metre",1]],ID["EPSG",32604]]'

    create_gdal_raster(rast, np.arange(9, dtype=np.int32).reshape(3, 3), crs=rast_crs)
    create_gdal_features(square, [make_rect(0.5, 0.5, 2.5, 2.5)], crs=vec_crs)

    with warnings.catch_warnings():
        warnings.simplefilter("error")
        exact_extract(rast, square, "mean")


@pytest.fixture()
def multidim_nc(tmp_path):

    gdal = pytest.importorskip("osgeo.gdal")

    fname = str(tmp_path / "test_multidim.nc")

    nx = 3
    ny = 4
    nt = 2

    drv = gdal.GetDriverByName("netCDF")

    ds = drv.CreateMultiDimensional(fname)
    rg = ds.GetRootGroup()

    x = rg.CreateDimension("longitude", None, None, nx)
    y = rg.CreateDimension("latitude", None, None, ny)
    t = rg.CreateDimension("time", None, None, nt)

    for dim in (x, y, t):
        values = np.arange(dim.GetSize()).astype(np.float32)

        if dim.GetName() != t.GetName():
            values += 0.5
        if dim.GetName() == y.GetName():
            values = np.flip(values)

        arr = rg.CreateMDArray(
            dim.GetName(), [dim], gdal.ExtendedDataType.Create(gdal.GDT_Float32)
        )
        arr.WriteArray(values)
        standard_name = arr.CreateAttribute(
            "standard_name", [], gdal.ExtendedDataType.CreateString()
        )
        standard_name.Write(dim.GetName())

    t2m = rg.CreateMDArray(
        "t2m", [t, y, x], gdal.ExtendedDataType.Create(gdal.GDT_Float32)
    )
    t2m_data = np.arange(nx * ny * nt).reshape((nt, ny, nx))
    t2m.WriteArray(t2m_data)

    tp_data = np.sqrt(t2m_data)
    tp = rg.CreateMDArray(
        "tp", [t, y, x], gdal.ExtendedDataType.Create(gdal.GDT_Float32)
    )
    tp.WriteArray(tp_data)

    return fname


@pytest.mark.parametrize("libname", ("gdal", "rasterio", "xarray"))
def test_gdal_multi_variable(multidim_nc, libname):

    square = make_rect(0.5, 0.5, 2.5, 2.5)

    rast = open_with_lib(multidim_nc, libname)

    results = exact_extract(rast, square, ["count", "sum"])

    assert results[0]["properties"] == pytest.approx(
        {
            "tp_band_1_sum": 10.437034457921982,
            "tp_band_2_sum": 17.4051194190979,
            "t2m_band_1_sum": 28.0,
            "tp_band_2_count": 4.0,
            "t2m_band_2_sum": 76.0,
            "tp_band_1_count": 4.0,
            "t2m_band_2_count": 4.0,
            "t2m_band_1_count": 4.0,
        }
    )


@pytest.mark.parametrize("strategy", ("feature-sequential", "raster-sequential"))
def test_gh_178(tmp_path, strategy):

    gdal = pytest.importorskip("osgeo.gdal")
    ogr = pytest.importorskip("osgeo.ogr")
    rxr = pytest.importorskip("rioxarray")

    raster_fname = str(tmp_path / "src.tif")
    poly_fname = str(tmp_path / "poly.shp")

    src_ds = gdal.GetDriverByName("GTiff").Create(raster_fname, 72, 34, 2)
    src_ds.SetGeoTransform(
        (
            117.09683458943421,
            8.983152841204135e-05,
            0.0,
            4.273195975028152,
            0.0,
            -8.983152841195037e-05,
        )
    )
    src_ds.GetRasterBand(1).Fill(1)
    del src_ds

    src_poly = gdal.GetDriverByName("ESRI Shapefile").Create(poly_fname, 0, 0, 0, 0)
    src_lyr = src_poly.CreateLayer("poly")
    src_feature = ogr.Feature(src_lyr.GetLayerDefn())
    src_feature.SetGeometry(
        ogr.CreateGeometryFromWkt(
            "POLYGON ((117.103213 4.271759,117.102853 4.271848,117.102853 4.272028,117.102314 4.272028,117.102224 4.272208,117.102045 4.272208,117.102045 4.271759,117.101775 4.271759,117.101775 4.271669,117.102045 4.271399,117.102045 4.271489,117.102314 4.271489,117.102224 4.271669,117.102404 4.271759,117.102404 4.27122,117.102584 4.27122,117.10265 4.271154,117.103074 4.271366,117.103033 4.271489,117.103123 4.271489,117.103213 4.271759))"
        )
    )
    src_lyr.CreateFeature(src_feature)
    del src_feature
    del src_lyr
    del src_poly

    rast = rxr.open_rasterio(
        raster_fname, masked=True, band_as_variable=True, parse_coordinates=True
    )

    results = exact_extract(rast, poly_fname, "count", strategy=strategy)

    assert results[0]["properties"]["band_1_count"] == pytest.approx(95.1929023920793)


@pytest.mark.parametrize("strategy", ("feature-sequential", "raster-sequential", "raster-parallel"))
def test_basic_stats_raster_parallel(strategy):
    rast = NumPyRasterSource(np.arange(1, 10, dtype=np.int32).reshape(3, 3))
    square = JSONFeatureSource(make_rect(0.5, 0.5, 2.5, 2.5))

    result = exact_extract(rast, square, "mean", strategy=strategy)

    assert result[0]["properties"]["mean"] == pytest.approx(5.0)


@pytest.mark.parametrize("strategy", ("feature-sequential", "raster-sequential", "raster-parallel"))
def test_coverage_area_raster_parallel(strategy):
    rast = NumPyRasterSource(np.arange(1, 10, dtype=np.int32).reshape(3, 3))

    square = JSONFeatureSource(make_rect(0.5, 0.5, 2.5, 2.5))

    result = exact_extract(
        rast,
        square,
        [
            "m1=mean",
            "m2=mean(coverage_weight=area_spherical_m2)",
            "c1=coverage",
            "c2=coverage(coverage_weight=area_spherical_m2)",
            "c3=coverage(coverage_weight=area_spherical_km2)",
            "c4=coverage(coverage_weight=area_cartesian)",
        ],
        strategy=strategy,
    )[0]["properties"]

    assert result["m2"] > result["m1"]

    assert np.all(np.isclose(result["c3"], result["c2"] * 1e-6))
    assert np.all(result["c4"] == result["c1"])


@pytest.mark.parametrize("strategy", ("feature-sequential", "raster-sequential", "raster-parallel"))
def test_multiple_stats_raster_parallel(strategy):
    rast = NumPyRasterSource(np.arange(1, 10).reshape(3, 3))
    square = JSONFeatureSource(make_rect(0.5, 0.5, 2.5, 2.5))

    result = exact_extract(rast, square, ("min", "max", "mean"), strategy=strategy)

    fields = result[0]["properties"]

    assert fields == {
        "min": 1,
        "max": 9,
        "mean": 5,
    }


def test_weighted_mean_raster_parallel_not_supported():
    rast = NumPyRasterSource(np.arange(1, 10).reshape(3, 3))
    weights = NumPyRasterSource(np.full((3, 3), 1))
    square = JSONFeatureSource(make_rect(0.5, 0.5, 2.5, 2.5))

    with pytest.raises(ValueError, match="Weighted operations are not supported with strategy='raster-parallel'"):
        exact_extract(rast, square, "weighted_mean", weights=weights, strategy="raster-parallel")


@pytest.mark.parametrize("stat", ("variance", "stdev", "coefficient_of_variation"))
def test_variance_operations_raster_parallel_not_supported(stat):
    rast = NumPyRasterSource(np.arange(1, 10).reshape(3, 3))
    square = JSONFeatureSource(make_rect(0.5, 0.5, 2.5, 2.5))

    with pytest.raises(ValueError, match="Variance operations are not supported with strategy='raster-parallel'"):
        exact_extract(rast, square, stat, strategy="raster-parallel")


@pytest.mark.parametrize("stat", ("mean", "sum", "stdev", "variance"))
def test_weighted_stats_raster_parallel_not_supported(stat):
    rast = NumPyRasterSource(np.arange(1, 10).reshape(3, 3))
    weights = NumPyRasterSource(np.full((3, 3), 1))
    square = JSONFeatureSource(make_rect(0.5, 0.5, 2.5, 2.5))

    weighted_stat = f"weighted_{stat}"

    with pytest.raises(ValueError, match="Weighted operations are not supported with strategy='raster-parallel'"):
        exact_extract(rast, square, weighted_stat, weights=weights, strategy="raster-parallel")


@pytest.mark.parametrize("threads", (1, 2, 4, 8))
def test_raster_parallel_threads(threads):
    rast = NumPyRasterSource(np.arange(1, 10, dtype=np.int32).reshape(3, 3))
    square = JSONFeatureSource(make_rect(0.5, 0.5, 2.5, 2.5))

    result = exact_extract(rast, square, "mean", strategy="raster-parallel", threads=threads)

    assert result[0]["properties"]["mean"] == pytest.approx(5.0)


def test_raster_parallel_threads_parameter_passed():
    from exactextract.processor import RasterParallelProcessor

    # Test that RasterParallelProcessor can be instantiated with different thread counts
    # This verifies the threads parameter is correctly accepted
    from exactextract.writer import JSONWriter
    from exactextract.feature import JSONFeatureSource
    import numpy as np

    rast = NumPyRasterSource(np.arange(1, 10, dtype=np.int32).reshape(3, 3))
    square = JSONFeatureSource(make_rect(0.5, 0.5, 2.5, 2.5))
    writer = JSONWriter()

    # Test with different thread counts
    for threads in [1, 2, 4, 7, 16]:
        proc = RasterParallelProcessor(square, writer, [], threads=threads)
        # Just verify it was created without error
