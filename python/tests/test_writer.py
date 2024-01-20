import pytest

from exactextract import GDALWriter, JSONFeature, JSONWriter, Operation, PandasWriter


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
    ogr = pytest.importorskip("osgeo.ogr")

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


def test_pandas_writer(np_raster_source, point_features):
    pd = pytest.importorskip("pandas")

    w = PandasWriter()

    w.add_column("id")
    w.add_operation(Operation("mean", "mean_result", np_raster_source))

    for f in point_features:
        f.feature["properties"]["mean_result"] = f.feature["id"] * f.feature["id"]

        # we are explicitly declaring columns (add_operation has been called)
        # but we have an unexpected column
        with pytest.raises(KeyError, match="type"):
            w.write(f)

    for f in point_features:
        del f.feature["properties"]["type"]

        w.write(f)

    df = w.features()

    assert isinstance(df, pd.DataFrame)
    assert len(df) == 2

    assert list(df.columns) == ["id", "mean_result"]
    assert list(df["id"]) == [3, 2]
    assert list(df["mean_result"]) == [9, 4]
