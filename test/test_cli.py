import csv
import json
import os
import re
import subprocess

import numpy as np
import pytest
from osgeo import gdal, gdal_array, ogr, osr

gdal.UseExceptions()


@pytest.fixture(scope="module")
def gdal_version():
    result = subprocess.run(["./exactextract", "--version"], stdout=subprocess.PIPE)
    match = re.search(b"GDAL (\\d+[.]\\d+[.]\\d+)", result.stdout)
    return tuple(int(x) for x in match[1].split(b"."))


@pytest.fixture()
def run(tmpdir):
    def runner(*args, return_fname=False, output_ext="csv", **kwargs):
        output_fname = tmpdir / f"out.{output_ext}"

        arglist = list(args)

        for arg_name, arg_values in kwargs.items():
            arg_name = arg_name.replace("_", "-")

            if isinstance(arg_values, (str, os.PathLike, bool)):
                arg_values = [arg_values]
            for v in arg_values:
                if v is True:
                    arglist += [f"--{arg_name}"]
                else:
                    arglist += [f"--{arg_name}", f"{v}"]

        cmd = [str(x) for x in ["./exactextract", "-o", output_fname] + arglist]

        if os.environ.get("USE_VALGRIND", "NO") == "YES":
            cmd.insert(0, "valgrind")

        cmd_str = " ".join([x if x.startswith("-") else f'"{x}"' for x in cmd])

        # print(cmd_str)

        try:
            subprocess.run(cmd, check=True)
            fail = False
        except subprocess.CalledProcessError:
            fail = True

        if fail:
            # avoid failing from "except" block to get a less verbose
            # failure message
            pytest.fail(f"Command failed: {cmd_str}")

        if return_fname:
            return output_fname

        with open(output_fname, "r") as f:
            reader = csv.DictReader(f)
            return [row for row in reader]

    return runner


@pytest.fixture()
def write_raster(tmp_path):

    files = []

    def writer(data, *, mask=None, nodata=None, scale=None, offset=None):

        fname = str(tmp_path / f"raster{len(files)}.tif")

        files.append(fname)

        if len(data.shape) == 3:
            nz, ny, nx = data.shape
        else:
            nz = 1
            ny, nx = data.shape

        drv = gdal.GetDriverByName("GTiff")
        ds = drv.Create(
            fname,
            xsize=nx,
            ysize=ny,
            bands=nz,
            eType=gdal_array.NumericTypeCodeToGDALTypeCode(data.dtype),
        )
        ds.SetGeoTransform([0, 1, 0, ny, 0, -1])

        if nz == 1:
            ds.GetRasterBand(1).WriteArray(data)
        else:
            for i in range(nz):
                ds.GetRasterBand(i + 1).WriteArray(data[i, :])

        for i in range(nz):
            if nodata:
                ds.GetRasterBand(i + 1).SetNoDataValue(nodata)
            if scale:
                ds.GetRasterBand(i + 1).SetScale(scale)
            if offset:
                ds.GetRasterBand(i + 1).SetOffset(offset)
            if mask is not None:
                ds.GetRasterBand(i + 1).CreateMaskBand(gdal.GMF_PER_DATASET)
                mb = ds.GetRasterBand(i + 1).GetMaskBand()
                assert mb.WriteArray(mask) == gdal.CE_None
                del mb

        return fname

    return writer


@pytest.fixture()
def write_features(tmp_path):

    output_fname = tmp_path / "shape.geojson"

    def writer(features, srs=None):
        drv = ogr.GetDriverByName("GeoJSON")
        ds = drv.CreateDataSource(str(output_fname))

        ogr_srs = osr.SpatialReference()
        if srs is not None:
            ogr_srs.ImportFromEPSG(srs)

        lyr = ds.CreateLayer("shape", ogr_srs)

        if type(features) is dict:
            features = [features]

        if features:
            for k in features[0].keys():
                field_defn = ogr.FieldDefn(k, ogr.OFTString)
                lyr.CreateField(field_defn)

            for feat in features:
                dst_feat = ogr.Feature(lyr.GetLayerDefn())
                for k, v in feat.items():
                    if k == "geom":
                        geom = ogr.CreateGeometryFromWkt(v)
                        dst_feat.SetGeometryDirectly(geom)
                    else:
                        dst_feat[k] = v

                lyr.CreateFeature(dst_feat)

        return output_fname

    return writer


@pytest.mark.parametrize("strategy", ("feature-sequential", "raster-sequential"))
def test_multiple_stats(strategy, run, write_raster, write_features):

    data = np.array([[1, 2, 3, 4], [1, 2, 2, 5], [3, 3, 3, 2]], np.int16)

    rows = run(
        polygons=write_features(
            {"id": 1, "geom": "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2, 0.5 2, 0.5 0.5))"}
        ),
        fid="id",
        raster=f"metric:{write_raster(data)}",
        stat=["mean(metric)", "variety(metric)", "quantile(metric, q=0.25)"],
        strategy=strategy,
    )

    assert len(rows) == 1
    assert list(rows[0].keys()) == [
        "id",
        "metric_mean",
        "metric_variety",
        "metric_quantile_25",
    ]
    assert float(rows[0]["metric_mean"]) == pytest.approx(2.16667, 1e-3)
    assert rows[0]["metric_variety"] == "3"


def test_implicit_syntax(run, write_raster, write_features):
    # Don't provide a name to the raster, and don't reference any raster names
    # in the stat function invocations

    data = np.array([[1, 2, 3, 4], [1, 2, 2, 5], [3, 3, 3, 2]], np.int16)

    rows = run(
        polygons=write_features(
            {"id": 1, "geom": "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2, 0.5 2, 0.5 0.5))"}
        ),
        fid="id",
        raster=write_raster(data),
        stat=["mean", "variety"],
    )

    assert len(rows) == 1

    # Since we only have one input raster with only one band, the constructed
    # column names don't include a raster name
    assert list(rows[0].keys()) == ["id", "mean", "variety"]
    assert float(rows[0]["mean"]) == pytest.approx(2.16667, 1e-3)
    assert rows[0]["variety"] == "3"


naming_test_cases = {
    # Values: Single-input band from a multi-band raster
    # Weights: None
    # Stats: No name references
    # Names include raster: No
    # Names include band: No
    "single_raster_single_band_unnamed": (
        "{}[2]",
        None,
        ["mean", "variety"],
        ["id", "mean", "variety"],
    ),
    # Values: Single band from a unnamed multi-band raster
    # Weights: Single band from a unnamed multi-band raster
    # Stats: No name references
    # Names include raster: No
    # Names include band: No
    "weighted_single_raster_single_band_unnamed": (
        "{}[1]",
        "{}[2]",
        ["weighted_mean", "variety"],
        ["id", "weighted_mean", "variety"],
    ),
    # Values: Unnamed multi-band raster
    # Weights: None
    # Stats: No name references
    # Names include raster: No
    # Names include band: No
    "single_raster_multiband_unnamed": (
        "{}",
        None,
        ["mean", "variety"],
        ["id", "band_1_mean", "band_2_mean", "band_1_variety", "band_2_variety"],
    ),
    # Values: Named multi-band raster
    # Weights: None
    # Stats: No name references
    # Names include raster: Yes
    # Names include band: Yes
    "single_raster_single_band_named": (
        "metric:{}",
        None,
        ["mean", "variety"],
        [
            "id",
            "metric_band_1_mean",
            "metric_band_2_mean",
            "metric_band_1_variety",
            "metric_band_2_variety",
        ],
    ),
    # Values: Named multi-band raster
    # Weights: Named single-band raster
    # Stats: No name references
    # Names include raster: Yes
    # Names include band: Yes
    "single_raster_weighted_multiband_named": (
        "metric:{}",
        "wx:{}[1]",
        ["weighted_mean", "variety"],
        [
            "id",
            "metric_band_1_wx_weighted_mean",
            "metric_band_2_wx_weighted_mean",
            "metric_band_1_variety",
            "metric_band_2_variety",
        ],
    ),
    # Values: Named single-band raster
    # Weights: None
    # Stats: Some reference names, some do not
    # Names include raster: only if no name reference
    # Names include band: Yes
    "single_raster_single_band_named_override": (
        "metric:{}",
        None,
        ["s=mean(metric_band_1)", "variety"],
        ["id", "s", "metric_band_1_variety", "metric_band_2_variety"],
    ),
    # Values: Two unnamed single-band rasters
    # Weights: None
    # Stats: No name references
    # Names include raster: Yes (constructed name)
    # Names include band: No
    "multiple_raster_single_band_unnamed": (
        ("{}[1]", "{}[1]"),
        None,
        ["mean", "count"],
        ["id", "raster0_mean", "raster1_mean", "raster0_count", "raster1_count"],
    ),
    # Values: Two unnamed multi-band rasters
    # Weights: None
    # Stats: No name references
    # Names include raster: Yes (constructed name)
    # Names include band: No
    "multiple_raster_multiband_unnamed": (
        ("{}", "{}"),
        None,
        ["mean", "count"],
        [
            "id",
            "raster0_band_1_mean",
            "raster0_band_2_mean",
            "raster1_band_1_mean",
            "raster1_band_2_mean",
            "raster0_band_1_count",
            "raster0_band_2_count",
            "raster1_band_1_count",
            "raster1_band_2_count",
        ],
    ),
    # We have two input rasters and specify a name for one, so we
    # so we derive a names from the filename of the other
    "multiple_raster_single_band_some_named": (
        ("{}[1]", "r0:{}[1]"),
        None,
        ["mean", "count(r0)"],
        ["id", "raster0_mean", "r0_mean", "r0_count"],
    ),
}


@pytest.mark.parametrize(
    "raster,weights,stats,expected_names",
    naming_test_cases.values(),
    ids=naming_test_cases.keys(),
)
def test_column_naming(
    run, write_raster, write_features, raster, weights, stats, expected_names
):

    data = np.array([[1, 2, 3, 4], [1, 2, 2, 5], [3, 3, 3, 2]], np.int16)
    data = np.stack((data, data * 2))

    if type(raster) is str:
        raster = [raster]

    if weights is None:
        weights = []
    elif type(weights) is str:
        weights = [weights]

    raster_args = [r.format(write_raster(data)) for r in raster]
    weight_args = [w.format(write_raster(data)) for w in weights]

    rows = run(
        polygons=write_features(
            {"id": 1, "geom": "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2, 0.5 2, 0.5 0.5))"}
        ),
        fid="id",
        raster=raster_args,
        weights=weight_args,
        stat=stats,
    )

    assert len(rows) == 1
    assert list(rows[0].keys()) == expected_names


def test_frac_output(run, write_raster, write_features):
    data = np.array([[1, 2, 3], [1, 2, 2], [3, 3, 3]], np.int16)

    rows = run(
        polygons=write_features(
            [
                {"id": 1, "geom": "POLYGON ((0 0, 3 0, 3 0.5, 0 0.5, 0 0))"},
                {
                    "id": 2,
                    "geom": "POLYGON ((1.5 1.5, 2.5 1.5, 2.5 2.5, 1.5 2.5, 1.5 1.5))",
                },
            ]
        ),
        fid="id",
        raster=f"class:{write_raster(data)}",
        stat=["unique", "frac"],
    )

    assert len(rows) == 3

    assert rows[0] == {"id": "1", "unique": "3", "frac": "1"}

    # order of rows for a given feature may vary between platforms
    rows = sorted(rows[1:], key=lambda x: x["unique"])
    assert rows[0] == {"id": "2", "unique": "2", "frac": "0.75"}
    assert rows[1] == {"id": "2", "unique": "3", "frac": "0.25"}


@pytest.mark.parametrize("strategy", ("feature-sequential", "raster-sequential"))
def test_feature_not_intersecting_raster(strategy, run, write_raster, write_features):

    data = np.array([[1, 2, 3], [1, 2, 2], [3, 3, 3]], np.float32)

    rows = run(
        polygons=write_features(
            [{"id": 1, "geom": "POLYGON ((100 100, 200 100, 200 200, 100 100))"}]
        ),
        fid="id",
        raster=f"value:{write_raster(data)}",
        stat=["count(value)", "mean(value)"],
        strategy=strategy,
    )

    assert len(rows) == 1
    assert rows[0] == {"id": "1", "value_count": "0", "value_mean": "nan"}


@pytest.mark.parametrize("strategy", ("feature-sequential", "raster-sequential"))
@pytest.mark.parametrize("dtype,nodata", [(np.float32, None), (np.int32, -999)])
def test_feature_intersecting_nodata(
    strategy, run, write_raster, write_features, dtype, nodata
):

    data = np.full((4, 3), nodata or np.nan, dtype=dtype)

    rows = run(
        polygons=write_features(
            {"id": 1, "geom": "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2, 0.5 2, 0.5 0.5))"}
        ),
        fid="id",
        raster=f"metric:{write_raster(data, nodata=nodata)}",
        stat=["count(metric)", "mean(metric)", "mode(metric)"],
        strategy=strategy,
    )

    assert len(rows) == 1
    assert rows[0] == {
        "id": "1",
        "metric_count": "0",
        "metric_mean": "nan",
        "metric_mode": "",
    }


@pytest.mark.parametrize("strategy", ("feature-sequential", "raster-sequential"))
def test_include_cols(strategy, run, write_raster, write_features):

    data = np.array([[1, 2, 3, 4], [1, 2, 2, 5], [3, 3, 3, 2]], np.int16)

    rows = run(
        polygons=write_features(
            [
                {
                    "id": 1,
                    "name": "A",
                    "class": 1,
                    "geom": "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2, 0.5 2, 0.5 0.5))",
                },
                {
                    "id": 2,
                    "name": "B",
                    "class": 2,
                    "geom": "POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0))",
                },
            ]
        ),
        fid="id",
        raster=f"metric:{write_raster(data)}",
        stat=["variety(metric)"],
        include_col=["name", "class"],
        strategy=strategy,
    )

    assert len(rows) == 2
    assert list(rows[0].keys()) == ["id", "name", "class", "metric_variety"]

    assert [row["name"] for row in rows] == ["A", "B"]
    assert [row["class"] for row in rows] == ["1", "2"]


def test_include_geom(run, write_raster, write_features):

    data = np.array([[1, 2, 3, 4], [1, 2, 2, 5], [3, 3, 3, 2]], np.int16)

    out = run(
        polygons=write_features(
            {
                "id": "3.14",
                "geom": "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2, 0.5 2, 0.5 0.5))",
            },
            srs=32145,
        ),
        fid="id",
        id_name="orig_id",
        id_type="float",
        raster=f"metric:{write_raster(data)}",
        stat=["y=mean(metric)"],
        include_geom=True,
        return_fname=True,
        output_ext="shp",
    )

    ds = ogr.Open(str(out))
    lyr = ds.GetLayer(0)
    f = lyr.GetNextFeature()
    g = f.GetGeometryRef()

    assert g.ExportToWkt().startswith("POLYGON")

    srs = lyr.GetSpatialRef()
    assert "Vermont" in srs.ExportToWkt()


@pytest.mark.parametrize("strategy", ("feature-sequential", "raster-sequential"))
def test_coverage_fractions(run, write_raster, write_features, strategy):

    data = np.arange(9, dtype=np.int32).reshape(3, 3)

    rows = run(
        polygons=write_features(
            {"id": 1, "geom": "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2.5, 0.5 2.5, 0.5 0.5))"}
        ),
        fid="id",
        raster=f"rast:{write_raster(data)}",
        stat=["coverage(rast)", "values(rast)"],
        strategy=strategy,
    )

    assert len(rows) == 9
    assert list(rows[0].keys()) == ["id", "rast_coverage", "rast_values"]

    assert rows[0]["id"] == "1"
    assert rows[0]["rast_coverage"] == "0.25"

    ids = [row["id"] for row in rows]
    assert ids == ["1"] * 9

    fracs = [float(row["rast_coverage"]) for row in rows]
    assert fracs == [0.25, 0.5, 0.25, 0.5, 1, 0.5, 0.25, 0.5, 0.25]


def test_coverage_fraction_args(run, write_raster, write_features):

    data = np.arange(9, dtype=np.int32).reshape(3, 3)

    rows = run(
        polygons=write_features(
            {
                "id": 1,
                "class": 3,
                "type": "B",
                "geom": "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2.5, 0.5 2.5, 0.5 0.5))",
            }
        ),
        fid="id",
        raster=f"rast:{write_raster(data)}",
        stat=[
            "coverage(rast)",
            "cell_id(rast)",
            "center_x(rast)",
            "center_y(rast)",
            "values(rast)",
        ],
        include_col=["class", "type"],
    )

    assert len(rows) == 9
    assert list(rows[0].keys()) == [
        "id",
        "class",
        "type",
        "rast_coverage",
        "rast_cell_id",
        "rast_center_x",
        "rast_center_y",
        "rast_values",
    ]

    assert [row["class"] for row in rows] == ["3"] * len(rows)

    assert [row["type"] for row in rows] == ["B"] * len(rows)

    cells = [row["rast_cell_id"] for row in rows]
    assert cells == ["0", "1", "2", "3", "4", "5", "6", "7", "8"]

    x = [float(row["rast_center_x"]) for row in rows]
    assert x == [0.5, 1.5, 2.5] * 3

    y = [float(row["rast_center_y"]) for row in rows]
    assert y == [2.5] * 3 + [1.5] * 3 + [0.5] * 3


def test_scale_offset(run, write_raster, write_features):

    data = np.array([[17650, 18085], [19127, 19428]], dtype=np.int16)
    scale = 0.001571273180860454
    offset = 250.47270984680802

    rows = run(
        polygons=write_features(
            {"id": 1, "geom": "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2, 0.5 2, 0.5 0.5))"}
        ),
        fid="id",
        raster=f"d2m:{write_raster(data, scale=scale, offset=offset)}",
        stat=["cell_id(d2m)", "values(d2m)"],
    )

    assert int(rows[0]["d2m_cell_id"]) == 0
    assert float(rows[0]["d2m_values"]) == pytest.approx(278.2057, rel=1e-3)


def test_scale_offset_nodata(run, write_raster, write_features):

    data = np.array([[1, 2, 3], [-999, -999, -999], [4, 5, -499]], dtype=np.int32)

    rows = run(
        polygons=write_features(
            {"id": 1, "geom": "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2.5, 0.5 2.5, 0.5 0.5))"}
        ),
        fid="id",
        raster=f"{write_raster(data, scale=2, offset=-1, nodata=-999)}",
        stat=["values"],
        nested_output=True,
    )

    values = np.fromstring(rows[0]["values"].strip("[").strip("]"), sep=",")
    np.testing.assert_array_equal(values, [1, 3, 5, 7, 9, -999])


def test_id_rename(run, write_raster, write_features):

    data = np.array([[1, 2, 3, 4], [1, 2, 2, 5], [3, 3, 3, 2]], np.int16)

    out = run(
        polygons=write_features(
            {
                "id": "3.14",
                "geom": "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2, 0.5 2, 0.5 0.5))",
            }
        ),
        fid="id",
        id_name="orig_id",
        id_type="float",
        raster=f"metric:{write_raster(data)}",
        stat=["mean(metric)"],
        return_fname=True,
    )

    lines = [line.strip() for line in open(out).readlines()]

    cols = lines[0].split(",")

    assert cols[0] == "orig_id"  # column was renamed

    values = lines[1].split(",")

    assert values[0] == "3.14"  # value is unquoted


def test_id_change_type(run, write_raster, write_features):

    data = np.array([[1, 2, 3, 4], [1, 2, 2, 5], [3, 3, 3, 2]], np.int16)

    out = run(
        polygons=write_features(
            {
                "id": "3.14",
                "geom": "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2, 0.5 2, 0.5 0.5))",
            }
        ),
        id_name="id",
        id_type="float",
        raster=f"metric:{write_raster(data)}",
        stat=["mean(metric)"],
        return_fname=True,
    )

    lines = [line.strip() for line in open(out).readlines()]

    cols = lines[0].split(",")

    assert cols[0] == "id"  # column was renamed

    values = lines[1].split(",")

    assert values[0] == "3.14"  # value is unquoted


def test_mask_band(run, write_raster, write_features):

    data = np.arange(1, 10, dtype=np.float32).reshape((3, 3))
    valid = np.array(
        [[False, False, False], [True, True, True], [False, False, False]]
    ).astype(np.int8)

    rows = run(
        polygons=write_features(
            {"geom": "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2, 0.5 2, 0.5 0.5))"}
        ),
        raster=write_raster(data, mask=valid),
        stat=["cell_id"],
        nested_output=True,
    )

    assert json.loads(rows[0]["cell_id"]) == [3, 4, 5]


@pytest.mark.parametrize(
    "dtype,val",
    [
        (np.uint8, 200),
        (np.uint16, 33000),
        (np.uint32, 2200000000),
        (np.int64, 2397083434877565865),
        (np.uint64, 2397083434877565865),  # cannot yet output values > range of int64
    ],
)
def test_gdal_types(gdal_version, dtype, val, run, write_raster, write_features):

    if dtype in {np.int64, np.uint64} and gdal_version < (3, 5, 0):
        pytest.skip("Int64 data type not supported in this version of GDAL")

    data = np.array([[val]], dtype=dtype)

    rows = run(
        polygons=write_features({"geom": "POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0))"}),
        raster=write_raster(data),
        stat="mode",
    )

    assert rows[0]["mode"] == str(val)
