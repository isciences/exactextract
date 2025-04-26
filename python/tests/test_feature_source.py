import pytest

from exactextract.feature import GDALFeatureSource, JSONFeatureSource


def test_gdal_feature_source():
    ogr = pytest.importorskip("osgeo.ogr")

    drv = ogr.GetDriverByName("Memory")
    ds = drv.CreateDataSource("")

    assert ds is not None

    lyr = ds.CreateLayer("test")
    lyr.CreateField(ogr.FieldDefn("id", ogr.OFTInteger))

    assert lyr is not None

    feat = ogr.Feature(lyr.GetLayerDefn())
    for i in range(5):
        feat["id"] = i
        lyr.CreateFeature(feat)

    fs = GDALFeatureSource(lyr)

    assert fs.count() == 5
    assert fs.count() == 5  # make sure we can iterate twice


def test_python_feature_source():
    features = []

    for i in range(5):
        features.append(
            {
                "id": i,
                "properties": {"name": f"feature_{i}"},
                "geometry": {"type": "Point", "coordinates": [i, i]},
            }
        )

    fs = JSONFeatureSource(features)

    assert fs.count() == 5
    assert fs.count() == 5
