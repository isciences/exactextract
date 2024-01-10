import pytest

from exactextract import Feature, GDALFeature, JSONFeature


@pytest.fixture()
def ogr_point():
    from osgeo import ogr

    fd = ogr.FeatureDefn()
    fd.AddFieldDefn(ogr.FieldDefn("area", ogr.OFTReal))
    fd.AddFieldDefn(ogr.FieldDefn("name", ogr.OFTString))

    ogr_feature = ogr.Feature(fd)
    ogr_feature["area"] = 33.6
    ogr_feature["name"] = "test"
    ogr_feature.SetGeometryDirectly(ogr.CreateGeometryFromWkt("POINT (15 22)"))

    return ogr_feature


def test_gdal_feature(ogr_point):
    f = GDALFeature(ogr_point)

    assert isinstance(f, Feature)

    assert f.check()

    f.set("area", 12.2)

    assert f.get("area") == 12.2
    assert ogr_point["area"] == 12.2


def test_json_feature():
    f = JSONFeature(
        {
            "id": 17,
            "properties": {"name": "Jay", "type": "q"},
            "geometry": {"type": "Point", "coordinates": [-74, 44]},
        }
    )

    assert isinstance(f, Feature)

    assert f.check()

    assert f.get("id") == 17
    assert f.get("type") == "q"

    f.set("id", 18)
    f.set("type", "n")

    assert f.get("id") == 18
    assert f.get("type") == "n"


def test_feature_copy_to(ogr_point):
    f = JSONFeature()
    GDALFeature(ogr_point).copy_to(f)

    ogr_point.geometry()

    assert f.get("area") == 33.6
    assert f.get("name") == "test"
