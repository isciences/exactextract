#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pytest

from exactextract import GDALWriter, JSONFeature, JSONWriter


@pytest.fixture()
def point_features():
    return [
        JSONFeature(
            {
                "id": 3,
                "properties": {"type": "apple"},
                "geometry": {"type": "Point", "coordinates": [3, 8]},
            }
        ),
        JSONFeature(
            {
                "id": 2,
                "properties": {"type": "pear"},
                "geometry": {"type": "Point", "coordinates": [2, 2]},
            }
        ),
    ]


def test_json_writer(point_features):
    w = JSONWriter()

    for f in point_features:
        w.write(f)

    assert len(w.features()) == len(point_features)

    f = w.features()[0]
    assert f["id"] == 3
    assert f["properties"]["type"] == "apple"

    f = w.features()[1]
    assert f["id"] == 2
    assert f["properties"]["type"] == "pear"


def test_gdal_writer(point_features):
    from osgeo import ogr

    drv = ogr.GetDriverByName("Memory")
    ds = drv.CreateDataSource("")

    w = GDALWriter(ds, "out")

    for f in point_features:
        w.write(f)
    w.finish()

    lyr = ds.GetLayerByName("out")
    assert lyr is not None
    assert lyr.GetFeatureCount() == len(point_features)

    f = lyr.GetNextFeature()
    assert f["id"] == 3
    assert f["type"] == "apple"

    f = lyr.GetNextFeature()
    assert f["id"] == 2
    assert f["type"] == "pear"
