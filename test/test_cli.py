import csv
import os
import pytest
import subprocess

import numpy as np
from osgeo import gdal, gdal_array, ogr

gdal.UseExceptions()

@pytest.fixture()
def run(tmpdir):

    def runner(*args, **kwargs):
        output_fname = tmpdir / "out.csv"

        arglist = list(args)

        for k, v in kwargs.items():
            k = k.replace('_', '-')

            if isinstance(v, (str, os.PathLike, bool)):
                v = [v]
            for x in v:
                if x is True:
                    arglist += [f"--{k}"]
                else:
                    arglist += [f"--{k}", f"{x}"]

        subprocess.run(['./exactextract', '-o', output_fname] + arglist,
                check=True)

        with open(output_fname, 'r') as f:
            reader = csv.DictReader(f)
            return [row for row in reader]

    return runner

@pytest.fixture()
def write_raster(tmp_path):

    files = []

    def writer(data):

        fname = str(tmp_path / f"raster{len(files)}.tif")

        files.append(fname)

        ny, nx = data.shape

        drv = gdal.GetDriverByName("GTiff")
        ds = drv.Create(fname, xsize=nx, ysize=ny, bands=1, eType=gdal_array.NumericTypeCodeToGDALTypeCode(data.dtype))
        ds.SetGeoTransform([0, 1, 0, ny, 0, -1])

        ds.GetRasterBand(1).WriteArray(data)

        return fname

    return writer

@pytest.fixture()
def write_features(tmp_path):

    output_fname = tmp_path / "shape.geojson"

    def writer(features):
        drv = ogr.GetDriverByName("GeoJSON")
        ds = drv.CreateDataSource(str(output_fname))

        lyr = ds.CreateLayer("shape")

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


def test_multiple_stats(run, write_raster, write_features):

    data = np.array([
        [1, 2, 3, 4],
        [1, 2, 2, 5],
        [3, 3, 3, 2]], np.int16)

    rows = run(
            polygons=write_features({"id": 1, "geom": "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2, 0.5 2, 0.5 0.5))"}),
            fid="id",
            raster=f"metric:{write_raster(data)}",
            stat=["mean(metric)", "variety(metric)"]
            )

    assert len(rows) == 1
    assert list(rows[0].keys()) == ['id', 'metric_mean', 'metric_variety']
    assert float(rows[0]['metric_mean']) == pytest.approx(2.16667, 1e-3)
    assert rows[0]['metric_variety'] == '3'


def test_coverage_fractions(run, write_raster, write_features):

    data = np.arange(9, dtype=np.int32).reshape(3, 3)

    rows = run(
            polygons=write_features({"id":1, "geom": "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2.5, 0.5 2.5, 0.5 0.5))"}),
            fid="id",
            raster=f"values:{write_raster(data)}",
            stat=["coverage(values)"]
        )

    assert len(rows) == 9
    assert list(rows[0].keys()) == ['id', 'coverage_fraction', 'values']

    assert rows[0]['id'] == '1'
    assert rows[0]['coverage_fraction'] == '0.25'

    ids = [row['id'] for row in rows]
    assert ids == ['1'] * 9

    fracs = [float(row['coverage_fraction']) for row in rows]
    assert fracs == [ 0.25, 0.5, 0.25, 0.5,  1, 0.5, 0.25, 0.5, 0.25 ]


def test_coverage_fraction_args(run, write_raster, write_features):

    data = np.arange(9, dtype=np.int32).reshape(3, 3)

    rows = run(
            polygons=write_features({"id":1, "geom": "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2.5, 0.5 2.5, 0.5 0.5))"}),
            fid="id",
            raster=f"values:{write_raster(data)}",
            stat=["coverage(values)"],
            include_cell=True,
            include_xy=True,
            include_area=True
        )

    assert len(rows) == 9
    assert list(rows[0].keys()) == ['id', 'cell', 'x', 'y', 'area', 'coverage_fraction', 'values']

    cells = [row['cell'] for row in rows]
    assert cells == ['0','1', '2', '3', '4', '5', '6', '7', '8']

    x = [float(row['x']) for row in rows]
    assert x == [0.5, 1.5, 2.5] * 3

    y = [float(row['y']) for row in rows]
    assert y == [2.5] * 3 + [1.5] * 3 + [0.5] * 3

    areas = [float(row['area']) for row in rows]
    assert areas == [1.0] * 9
