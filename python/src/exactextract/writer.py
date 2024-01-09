import copy

from _exactextract import Writer

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


class GDALWriter(Writer):
    def __init__(self, ds, name=""):
        super().__init__()
        self.feature_list = []
        self.ds = ds
        self.layer_name = name
        self.prototype = {"type": "Feature", "properties": {}}

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

    def finish(self):
        from osgeo import ogr

        fields = self._collect_fields()

        lyr = self.ds.CreateLayer(self.layer_name)
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
