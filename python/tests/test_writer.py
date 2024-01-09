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
    ogr = pytest.importorskip("ogr")

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


def test_pandas_writer_columns_discovered(point_features):
    pd = pytest.importorskip("pandas")

    w = PandasWriter()

    for f in point_features:
        w.write(f)
    w.finish()

    df = w.features()

    assert isinstance(df, pd.DataFrame)
    assert len(df) == 2

    assert list(df.columns) == ["id", "type"]

    assert list(df["id"]) == [3, 2]
    assert list(df["type"]) == ["apple", "pear"]


def test_pandas_writer_columns_declared(np_raster_source, point_features):
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


def test_pandas_writer_columns_declared_and_discovered(
    np_raster_source, point_features
):
    pd = pytest.importorskip("pandas")

    w = PandasWriter()

    w.add_column("id")
    w.add_column("type")
    w.add_operation(Operation("mean", "mean_result", np_raster_source))
    w.add_operation(Operation("frac", "", np_raster_source))

    point_features[0].feature["properties"]["mean_result"] = 9
    point_features[0].feature["properties"]["frac_3"] = 3

    point_features[1].feature["properties"]["mean_result"] = 4
    point_features[1].feature["properties"]["frac_3"] = 33
    point_features[1].feature["properties"]["frac_4"] = 44

    for f in point_features:
        w.write(f)

    df = w.features()

    assert isinstance(df, pd.DataFrame)
    assert len(df) == 2

    assert list(df.columns) == ["id", "type", "mean_result", "frac_3", "frac_4"]

    assert list(df["id"]) == [3, 2]
    assert list(df["type"]) == ["apple", "pear"]
    assert list(df["mean_result"]) == [9, 4]
    assert list(df["frac_3"]) == [3, 33]
    assert list(df["frac_4"]) == pytest.approx([float("nan"), 44], abs=0, nan_ok=True)
