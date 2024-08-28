import os

from ._exactextract import Feature
from ._exactextract import FeatureSource as _FeatureSource

__all__ = [
    "FeatureSource",
    "GDALFeatureSource",
    "JSONFeatureSource",
    "GeoPandasFeatureSource",
    "QGISFeatureSource",
]


class FeatureSource(_FeatureSource):
    """
    Source from which polygon features can be read.

    Several implementations are included in exactextract:

    - :py:class:`GDALFeatureSource`
    - :py:class:`JSONFeatureSource`
    - :py:class:`GeoPandasFeatureSource`
    - :py:class:`QGISFeatureSource`
    """

    def __init__(self):
        super().__init__()


class GDALFeatureSource(FeatureSource):
    """
    A FeatureSource using the GDAL/OGR Python interface to provide features.
    """

    def __init__(self, src):
        """
        Args:
            src: one of the following
                 - string or Path to a file/datasource that can be opened with GDAL/OGR
                 - a ``gdal.Dataset``
                 - an ``ogr.DataSource``
                 - an ``ogr.Layer``
                 If the file has more than one layer, e.g., a GeoPackage, an
                 ``ogr.Layer`` must be provided directly.
        """
        super().__init__()

        if isinstance(src, (str, os.PathLike)):
            from osgeo import ogr

            src = ogr.Open(src)

        try:
            nlayers = src.GetLayerCount()
        except AttributeError:
            self.src = src
        else:
            self.ds = src  # keep a reference to ds
            if nlayers > 1:
                raise Exception(
                    "Can only process a single layer; call directly with ogr.Layer object."
                )

            self.src = src.GetLayer(0)

    def count(self):
        return self.src.GetFeatureCount()

    def __iter__(self):
        for f in self.src:
            yield GDALFeature(f)

    def srs_wkt(self):
        srs = self.src.GetSpatialRef()
        if srs:
            return srs.ExportToWkt()


class GDALFeature(Feature):
    def __init__(self, f):
        Feature.__init__(self)
        self.feature = f

    def set(self, name, value):
        idx = self.feature.GetDefnRef().GetFieldIndex(name)

        if type(value) in (int, float, str):
            self.feature.SetField(idx, value)
            return

        import numpy as np

        if value.dtype == np.int64:
            self.feature.SetFieldInteger64List(idx, value)
        elif value.dtype == np.int32:
            self.feature.SetFieldIntegerList(idx, value)
        elif value.dtype == np.float64:
            self.feature.SetFieldDoubleList(idx, value)
        else:
            raise Exception("Unexpected type in GDALFeature::set")

    def get(self, name):
        return self.feature.GetField(name)

    def geometry(self):
        return bytes(self.feature.GetGeometryRef().ExportToWkb())

    def set_geometry(self, wkb):
        if wkb:
            from osgeo import ogr

            geom = ogr.CreateGeometryFromWkb(wkb)
            self.feature.SetGeometryDirectly(geom)
        elif self.feature.GetGeometryRef():
            self.feature.SetGeometry(None)

    def set_geometry_format(self):
        return "wkb"

    def fields(self):
        defn = self.feature.GetDefnRef()
        return [defn.GetFieldDefn(i).GetName() for i in range(defn.GetFieldCount())]


class JSONFeatureSource(FeatureSource):
    """
    A FeatureSource providing GeoJSON-like features.
    """

    def __init__(self, src, *, srs_wkt=None):
        super().__init__()
        if type(src) is dict:
            self.src = [src]
        else:
            self.src = src

        self.srs_wkt_txt = srs_wkt

    def count(self):
        return len(self.src)

    def __iter__(self):
        for f in self.src:
            yield JSONFeature(f)

    def srs_wkt(self):
        return self.srs_wkt_txt


class JSONFeature(Feature):
    def __init__(self, f=None):
        Feature.__init__(self)
        if f is None:
            self.feature = {"type": "Feature", "properties": {}}
        elif hasattr(f, "__geo_interface__"):
            self.feature = f.__geo_interface__
        else:
            self.feature = f

    def set(self, name, value):
        if name == "id":
            self.feature["id"] = value
        else:
            self.feature["properties"][name] = value

    def get(self, name):
        if name == "id":
            return self.feature["id"]
        else:
            return self.feature["properties"][name]

    def geometry(self):
        if "geometry" in self.feature:
            import json

            return json.dumps(self.feature["geometry"])

    def set_geometry(self, geojson):
        if geojson:
            import json

            self.feature["geometry"] = json.loads(geojson)
        elif "geometry" in self.feature:
            del self.feature["geometry"]

    def set_geometry_format(self):
        return "geojson"

    def fields(self):
        fields = []
        if "id" in self.feature:
            fields.append("id")
        if "properties" in self.feature:
            fields += self.feature["properties"].keys()
        return fields


class GeoPandasFeatureSource(FeatureSource):
    """
    A FeatureSource using GeoPandas GeoDataFrame to provide features.
    """

    def __init__(self, src):
        super().__init__()
        self.src = src

    def count(self):
        return len(self.src.index)

    def __iter__(self):
        for f in self.src.iterfeatures():
            yield JSONFeature(f)

    def srs_wkt(self):
        return self.src.crs.to_wkt()


class QGISFeature(Feature):
    def __init__(self, feature=None, *, fields=None):
        super().__init__()
        if feature:
            self.feature = feature
        else:
            from qgis.core import QgsFeature

            # create a QgsFeature and set fields
            self.feature = QgsFeature()
            self.feature.setFields(fields)

    def geometry(self):
        return bytes(self.feature.geometry().asWkb())

    def get(self, name):
        return self.feature[name]

    def fields(self):
        return [field.name() for field in self.feature.fields()]

    def set_geometry(self, wkb):
        from qgis.core import QgsGeometry

        geometry = QgsGeometry()
        geometry.fromWkb(wkb)
        self.feature.setGeometry(geometry)

    def set_geometry_format(self):
        return "wkb"

    def set(self, name, value):
        # populate QgsFeature with values
        import numpy as np

        if type(value) is np.ndarray:
            value = self.convert_ndarray_to_json(name, value)
        self.feature.setAttribute(name, value)

    @staticmethod
    def convert_ndarray_to_json(name, array):
        import json

        import numpy as np

        class NumpyEncoder(json.JSONEncoder):
            def default(self, obj):
                if isinstance(obj, np.ndarray):
                    return obj.tolist()
                return json.JSONEncoder.default(self, obj)

        return json.dumps({name: array}, cls=NumpyEncoder)


class QGISFeatureSource(FeatureSource):
    """FeatureSource providing features from a QGIS vector layer."""

    def __init__(self, src):
        super().__init__()
        self.src = src

    def count(self):
        return self.src.featureCount()

    def __iter__(self):
        for f in self.src.getFeatures():
            yield QGISFeature(f)

    def srs_wkt(self):
        return self.src.sourceCrs().toWkt()
