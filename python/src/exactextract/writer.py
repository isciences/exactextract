import copy

from ._exactextract import Writer
from .feature import GDALFeature, JSONFeature


class JSONWriter(Writer):
    def __init__(self):
        super().__init__()
        self.feature_list = []

    def write(self, feature):
        f = JSONFeature()
        feature.copy_to(f)
        self.feature_list.append(f.feature)

    def features(self):
        return self.feature_list


class PandasWriter(Writer):
    def __init__(self, *, srs_wkt=None):
        super().__init__()

        self.fields = {}
        self.geoms = []
        self.srs_wkt = srs_wkt

    def add_operation(self, op):
        self.fields[op.name] = []

    def add_column(self, col_name):
        self.fields[col_name] = []

    def add_geometry(self):
        self.fields["geometry"] = []

    def write(self, feature):
        f = JSONFeature()
        feature.copy_to(f)

        for field_name, value in f.feature["properties"].items():
            self.fields[field_name].append(value)
        if "id" in f.feature:
            self.fields["id"].append(f.feature["id"])
        if "geometry" in self.fields and "geometry" in f.feature:
            import shapely

            self.fields["geometry"].append(
                shapely.geometry.shape(f.feature["geometry"])
            )

    def features(self):
        if "geometry" in self.fields:
            import geopandas as gpd

            return gpd.GeoDataFrame(self.fields, geometry="geometry", crs=self.srs_wkt)
        else:
            import pandas as pd

            return pd.DataFrame(self.fields, copy=False)


class GDALWriter(Writer):
    def __init__(self, ds, name="", *, srs_wkt=None):
        super().__init__()
        self.feature_list = []
        self.ds = ds
        self.layer_name = name
        self.prototype = {"type": "Feature", "properties": {}}
        self.srs_wkt = srs_wkt

    def add_operation(self, op):
        # Create a prototype feature so that field names
        # match the order they are specified in input
        # operations.
        self.prototype["properties"][op.name] = None

    def add_column(self, col_name):
        self.prototype["properties"][col_name] = None

    def write(self, feature):
        f = JSONFeature(copy.deepcopy(self.prototype))
        feature.copy_to(f)
        self.feature_list.append(f)

    def features(self):
        return None

    def finish(self):
        from osgeo import ogr, osr

        fields = self._collect_fields()

        srs = osr.SpatialReference()
        if self.srs_wkt:
            srs.ImportFromWkt(self.srs_wkt)

        lyr = self.ds.CreateLayer(self.layer_name, srs)
        for field_def in fields.values():
            lyr.CreateField(field_def)

        for feature in self.feature_list:
            ogr_feature = ogr.Feature(lyr.GetLayerDefn())
            feature.copy_to(GDALFeature(ogr_feature))
            lyr.CreateFeature(ogr_feature)

    def _collect_fields(self):
        from osgeo import ogr

        ogr_fields = {}

        for feature in self.feature_list:
            for field_name in feature.fields():
                if field_name not in ogr_fields:
                    field_type = None

                    value = feature.get(field_name)

                    if type(value) is str:
                        field_type = ogr.OFTString
                    elif type(value) is float:
                        field_type = ogr.OFTReal
                    elif type(value) is int:
                        field_type = ogr.OFTInteger

                    ogr_fields[field_name] = ogr.FieldDefn(field_name, field_type)

        return ogr_fields
