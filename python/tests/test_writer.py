import json

import pytest
from exactextract import Operation
from exactextract.feature import JSONFeature
from exactextract.writer import GDALWriter, JSONWriter, PandasWriter, QGISWriter


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
    np = pytest.importorskip("numpy")
    ogr = pytest.importorskip("osgeo.ogr")

    drv = ogr.GetDriverByName("Memory")
    ds = drv.CreateDataSource("")

    w = GDALWriter(ds, layer_name="out")

    features = []
    for i in range(3):
        features.append(
            JSONFeature(
                {
                    "type": "Feature",
                    "id": i + 2,
                    "properties": {
                        "int_field": i + 13,
                        "float_field": float(i) + 13.7,
                        "str_field": "apple",
                        "int_array_field": np.array([1, 3, i], dtype=np.int32),
                        "int64_array_field": np.array([1, 3, i], dtype=np.int64),
                        "float_array_field": np.array(
                            [13.7, 26.4, i], dtype=np.float64
                        ),
                    },
                    "geometry": {"type": "Point", "coordinates": [i, i + 1]},
                }
            )
        )

    for f in features:
        w.write(f)
    w.finish()

    lyr = ds.GetLayerByName("out")
    assert lyr is not None
    assert lyr.GetFeatureCount() == len(features)

    defn = lyr.GetLayerDefn()
    assert (
        defn.GetFieldDefn(defn.GetFieldIndex("int_field")).GetType() == ogr.OFTInteger
    )
    assert defn.GetFieldDefn(defn.GetFieldIndex("float_field")).GetType() == ogr.OFTReal
    assert defn.GetFieldDefn(defn.GetFieldIndex("str_field")).GetType() == ogr.OFTString
    assert (
        defn.GetFieldDefn(defn.GetFieldIndex("int_array_field")).GetType()
        == ogr.OFTIntegerList
    )
    assert (
        defn.GetFieldDefn(defn.GetFieldIndex("int64_array_field")).GetType()
        == ogr.OFTInteger64List
    )
    assert (
        defn.GetFieldDefn(defn.GetFieldIndex("float_array_field")).GetType()
        == ogr.OFTRealList
    )

    for f_in in features:
        f_out = lyr.GetNextFeature()
        assert f_out["id"] == f_in.feature["id"]
        for field, value in f_in.feature["properties"].items():
            if type(value) is np.ndarray:
                assert np.array_equal(f_out[field], value)
                # GDAL python bindings return a list, not a np.ndarray,
                # so we can't check the dtype of the returned value.
                # However, we have already checked the OGR data type above.
                # assert f_out[field].dtype == value.dtyp
            else:
                assert f_out[field] == value

        assert (
            json.loads(f_out.GetGeometryRef().ExportToJson())
            == f_in.feature["geometry"]
        )


def test_pandas_writer(np_raster_source, point_features):
    pd = pytest.importorskip("pandas")

    w = PandasWriter()

    w.add_column("id")
    w.add_operation(Operation("mean", "mean_result", np_raster_source))

    for f in point_features:
        f.feature["properties"]["mean_result"] = f.feature["id"] * f.feature["id"]
        w.write(f)

    df = w.features()

    assert isinstance(df, pd.DataFrame)
    assert len(df) == 2

    assert list(df.columns) == ["id", "mean_result"]
    assert list(df["id"]) == [3, 2]
    assert list(df["mean_result"]) == [9, 4]


def test_qgis_writer(np_raster_source, point_features):
    qgis_core = pytest.importorskip("qgis.core")

    w = QGISWriter()

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

    qgs_vector = w.features()

    qgs_features = list(qgs_vector.getFeatures())

    assert isinstance(qgs_vector, qgis_core.QgsVectorLayer)
    assert qgs_vector.fields().count() == 2

    assert qgs_vector.fields().names() == ["id", "mean_result"]
    assert len(qgs_features) == 2
    assert qgs_features[0].attributes() == [3, 9]
    assert qgs_features[1].attributes() == [2, 4]
    assert qgs_features[0].hasGeometry()
    assert qgs_features[0].geometry().asWkt() == "Point (3 8)"
    assert qgs_features[1].hasGeometry()
    assert qgs_features[1].geometry().asWkt() == "Point (2 2)"
