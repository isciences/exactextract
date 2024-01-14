import csv
import os
import subprocess

import numpy as np
import pytest
from osgeo import gdal, gdal_array, ogr, osr

gdal.UseExceptions()


@pytest.fixture()
def run(tmpdir):
    def runner(*args, return_fname=False, output_ext="csv", **kwargs):
        output_fname = tmpdir / f"out.{output_ext}"

        arglist = list(args)

        for k, v in kwargs.items():
            k = k.replace("_", "-")

            if isinstance(v, (str, os.PathLike, bool)):
                v = [v]
            for x in v:
                if x is True:
                    arglist += [f"--{k}"]
                else:
                    arglist += [f"--{k}", f"{x}"]

        cmd = [str(x) for x in ["./exactextract", "-o", output_fname] + arglist]

        cmd_str = " ".join([x if x.startswith("-") else f'"{x}"' for x in cmd])
        print(cmd_str)

        subprocess.run(cmd, check=True)

        if return_fname:
            return output_fname

        with open(output_fname, "r") as f:
            reader = csv.DictReader(f)
            return [row for row in reader]

    return runner


@pytest.fixture()
def write_raster(tmp_path):

    files = []

    def writer(data, nodata=None, scale=None, offset=None):

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
        stat=["mean(metric)", "variety(metric)", "quantile(q=0.25)"],
        strategy=strategy,
    )

    assert len(rows) == 1
    assert list(rows[0].keys()) == ["id", "metric_mean", "metric_variety", "q25"]
    assert float(rows[0]["metric_mean"]) == pytest.approx(2.16667, 1e-3)
    assert rows[0]["metric_variety"] == "3"


def test_implicit_syntax(run, write_raster, write_features):

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
    assert list(rows[0].keys()) == ["id", "mean", "variety"]
    assert float(rows[0]["mean"]) == pytest.approx(2.16667, 1e-3)
    assert rows[0]["variety"] == "3"


@pytest.mark.parametrize(
    "raster,stats,expected_names",
    (
        (
            "{}",
            ["mean", "variety"],
            ["id", "band_1_mean", "band_2_mean", "band_1_variety", "band_2_variety"],
        ),
        (
            "metric:{}",
            ["mean", "variety"],
            [
                "id",
                "metric_band_1_mean",
                "metric_band_2_mean",
                "metric_band_1_variety",
                "metric_band_2_variety",
            ],
        ),
        (
            "metric:{}",
            ["s=mean(metric_band_1)", "variety"],
            ["id", "s", "metric_band_1_variety", "metric_band_2_variety"],
        ),
        ("{}[2]", ["mean", "variety"], ["id", "mean", "variety"]),
    ),
)
def test_multiband_unweighted(
    run, write_raster, write_features, raster, stats, expected_names
):

    data = np.array([[1, 2, 3, 4], [1, 2, 2, 5], [3, 3, 3, 2]], np.int16)
    data = np.stack((data, data * 2))

    rows = run(
        polygons=write_features(
            {"id": 1, "geom": "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2, 0.5 2, 0.5 0.5))"}
        ),
        fid="id",
        raster=raster.format(write_raster(data)),
        stat=stats,
    )

    assert len(rows) == 1
    assert list(rows[0].keys()) == expected_names


# def test_multiband_weighted(
#    run,
#    write_raster,
#    write_features,
#    value_bands,
#    weight_bands,
#    raster,
#    stats,
#    expected_names,
# ):
#    pass


@pytest.mark.parametrize(
    "rasters,stats,expected_names",
    (
        (
            ("{}", "{}"),
            ["mean", "count"],
            ["id", "raster0_mean", "raster1_mean", "raster0_count", "raster1_count"],
        ),
        (
            ("{}", "r1:{}"),
            ["mean", "count(r1)"],
            ["id", "raster0_mean", "r1_mean", "r1_count"],
        ),
    ),
)
def test_multiple_rasters(
    run, write_raster, write_features, rasters, stats, expected_names
):
    data1 = np.arange(12, dtype=np.int32).reshape(3, 4)
    data2 = np.sqrt(data1)

    rows = run(
        polygons=write_features(
            {"id": 1, "geom": "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2, 0.5 2, 0.5 0.5))"}
        ),
        fid="id",
        raster=[
            rasters[0].format(write_raster(data1)),
            rasters[1].format(write_raster(data2)),
        ],
        stat=stats,
    )

    assert list(rows[0].keys()) == expected_names


def test_stats_deferred(run, write_raster, write_features):
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
        stat="frac(class)",
    )

    assert len(rows) == 2
    assert list(rows[0].keys()) == ["id", "frac_2", "frac_3"]

    assert rows[0]["frac_2"] == ""
    assert float(rows[0]["frac_3"]) == 1

    assert float(rows[1]["frac_2"]) == 0.75
    assert float(rows[1]["frac_3"]) == 0.25


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
        raster=f"metric:{write_raster(data, nodata)}",
        stat=["count(metric)", "mean(metric)", "mode(metric)"],
        strategy=strategy,
    )

    assert len(rows) == 1
    assert rows[0] == {
        "id": "1",
        "metric_count": "0",
        "metric_mean": "nan",
        "metric_mode": str(nodata) if nodata else "nan",
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
