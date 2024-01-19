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
        self.feature_count = 0
        self.columns_known = None
        self.geoms = []
        self.srs_wkt = srs_wkt

    def add_operation(self, op):
        if self.columns_known is None:
            self.columns_known = True

        if op.column_names_known:
            for field_name in op.field_names():
                self.fields[field_name] = []
        else:
            self.columns_known = False

    def add_column(self, col_name):
        if self.columns_known is None:
            self.columns_known = True
        self.fields[col_name] = []

    def add_geometry(self):
        self.fields["geometry"] = []

    def _add_missing_columns(self, f):
        if self.columns_known:
            return
        if "id" in f and "id" not in self.fields:
            self.fields["id"] = [None] * self.feature_count
        for field_name in f["properties"]:
            if field_name not in self.fields:
                self.fields[field_name] = [None] * self.feature_count

    def _pad_missing_values(self):
        if self.columns_known:
            return
        for field in self.fields.values():
            if len(field) < self.feature_count:
                field.append(None)

    def write(self, feature):
        f = JSONFeature()
        feature.copy_to(f)

        self._add_missing_columns(f.feature)

        for field_name, value in f.feature["properties"].items():
            self.fields[field_name].append(value)
        if "id" in f.feature:
            self.fields["id"].append(f.feature["id"])
        if "geometry" in self.fields and "geometry" in f.feature:
            import shapely

            self.fields["geometry"].append(
                shapely.geometry.shape(f.feature["geometry"])
            )

        self.feature_count += 1

        self._pad_missing_values()

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
        for field_name in op.field_names():
            self.prototype["properties"][field_name] = None

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
