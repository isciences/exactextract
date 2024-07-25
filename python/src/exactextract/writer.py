import copy
import os
from typing import Mapping, Optional, Tuple

from ._exactextract import Writer as _Writer
from .feature import GDALFeature, JSONFeature, QGISFeature


class Writer(_Writer):
    """Writes the results of summary operations to a desired format"""

    def __init__(self):
        super().__init__()


class JSONWriter(Writer):
    """
    Creates GeoJSON-like features
    """

    def __init__(
        self,
        *,
        array_type: str = "numpy",
        map_fields: Optional[Mapping[str, Tuple[str]]] = None
    ):
        """
        Create a new JSONWriter.

        Args:
            array_type: type that should be used to represent array outputs.
                        Either "numpy" (default) or "list".
            map_fields: An optional dictionary of fields to be created by
                        interpreting one field as keys and another as values, in the format
                        ``{ dst_field : (src_keys, src_vals) }``. For example, the fields
                        "values" and "frac" would be combined into a field called
                        "frac_map" using ``map_fields = {"frac_map": ("values", "frac")}``.
        """
        super().__init__()

        if array_type not in ("numpy", "list"):
            raise ValueError("Unsupported array_type: " + array_type)

        self.array_type = array_type
        self.feature_list = []
        self.map_fields = map_fields or {}
        self.op_fields = {}
        self.ops = []

    def add_operation(self, op):
        self.ops.append(op)

    def write(self, feature):
        f = JSONFeature()
        feature.copy_to(f)

        self._convert_arrays(f)
        self._create_map_fields(f)

        self.feature_list.append(f.feature)

    def features(self):
        return self.feature_list

    def _convert_arrays(self, f):
        props = f.feature["properties"]

        if self.array_type == "list":
            import numpy as np

            for k in props:
                if type(props[k]) is np.ndarray:
                    props[k] = list(props[k])

    def _fields_for_stat(self, stat):
        return [o.name for o in self.ops if o.stat == stat]

    def _create_map_fields(self, f):
        props = f.feature["properties"]

        new_fields = {}
        to_delete = set()
        for field in self.map_fields:
            key_stat, val_stat = self.map_fields[field]

            key_fields = self._fields_for_stat(key_stat)
            val_fields = self._fields_for_stat(val_stat)

            assert len(key_fields) == len(val_fields)

            for key_field, val_field in zip(key_fields, val_fields):
                new_field = val_field.replace(val_stat, field)

                new_fields[new_field] = {
                    k: v for k, v in zip(props[key_field], props[val_field])
                }

                to_delete.add(key_field)
                to_delete.add(val_field)
        for field in to_delete:
            del props[field]
        props.update(new_fields)


class PandasWriter(Writer):
    """Creates a (Geo)Pandas DataFrame"""

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


class QGISWriter(Writer):
    """Creates QGSVectorLayer"""

    def __init__(self, *, srs_wkt=None):
        super().__init__()
        from qgis.core import QgsCoordinateReferenceSystem

        self.fields = []
        self.feats = []
        self.geom_type = "unknown"

        self.crs = QgsCoordinateReferenceSystem()
        self.crs.createFromWkt(srs_wkt)
        self.vector_layer = None

    def add_operation(self, op):
        self.fields.append(op.name)

    def add_column(self, col_name):
        self.fields.append(col_name)

    def write(self, feature):
        if not self.vector_layer:
            # test every property vs fields type and set correct type
            tmp_feature = JSONFeature()
            feature.copy_to(tmp_feature)
            self.fields = self._set_fields_types(self.fields, tmp_feature)
            self.fields = self._convert_to_QgsFields(self.fields)
            self.geom_type = self._get_geometry_type(tmp_feature)
            self.vector_layer, self.data_provider = self._create_vector_layer()

        qgis_feature = QGISFeature(fields=self.fields)
        feature.copy_to(qgis_feature)

        self.data_provider.addFeature(qgis_feature.feature)

    def features(self):
        self.vector_layer.updateExtents()
        return self.vector_layer

    def _create_vector_layer(self):
        """
        Creates a vector layer with the defined geometry type, CRS and fields.
        Returns the created layer as well as its data provider.

        Returns:
            Tuple[QgsVectorLayer, QgsVectorDataProvider]: initialized vector layer, data provider for created vector layer
        """
        from qgis.core import QgsVectorLayer

        if self.geom_type == "unknown":
            raise TypeError("Unknown geometry type!")
        vector_layer = QgsVectorLayer(
            self.geom_type + "?crs=" + self.crs.authid(), "temporary_layer", "memory"
        )
        data_provider = vector_layer.dataProvider()

        # add fields
        data_provider.addAttributes(self.fields)
        vector_layer.updateFields()  # tell the vector layer to fetch changes from the provider

        return vector_layer, data_provider

    @staticmethod
    def _get_geometry_type(feature: JSONFeature) -> str:
        """
        get geometry type of an input feature geometry

        Args:
            feature (JSONFeature): feature with geometry field

        Returns:
            str: describes geometry type (point, linestring, polygon, unknown)
        """
        if "geometry" in feature.feature:
            geom_type = feature.feature["geometry"]["type"].replace("Multi", "").lower()

            if geom_type in {"point", "linestring", "polygon"}:
                return geom_type
            else:
                return "unknown"

    @staticmethod
    def _convert_to_QgsFields(fields_list: list):
        """
        Converts a list with `QgsField` into a `QgsFields` object

        Args:
            fields_list (list): list with `QgsField`

        Returns:
            QgsFields: `QgsFields` object with the same fields as in input list
        """
        from qgis.core import QgsFields

        result_fields = QgsFields()

        for qgs_field in fields_list:
            result_fields.append(qgs_field)
        return result_fields

    @staticmethod
    def _set_fields_types(fields_list: list[str], feature: JSONFeature) -> list:
        """
        Sets field types based on provided data. If no type is specified it will be set as string by default
        with type_name set as "Unknown".

        Args:
            fields_list (list[str]): list with fields names
            feature (JSONFeature): feature with values that will be tested

        Returns:
            list: list with `QgsField`
        """
        import numpy
        from qgis.core import QgsField
        from qgis.PyQt.QtCore import QVariant

        mod_fields_list = []
        for field_name in fields_list:
            value = feature.get(field_name)

            if type(value) is str:
                field_type = QVariant.String
                type_name = "String"
            elif type(value) is float:
                field_type = QVariant.Double
                type_name = "Double"
            elif type(value) is int:
                field_type = QVariant.Int
                type_name = "Int"
            elif type(value) is numpy.ndarray:
                field_type = QVariant.String
                type_name = "Array"
            else:
                field_type = QVariant.String
                type_name = "Unknown"

            mod_fields_list.append(QgsField(field_name, field_type, type_name))

        return mod_fields_list


class GDALWriter(Writer):
    """Writes results using GDAL/OGR"""

    def __init__(
        self, dataset=None, *, filename=None, driver=None, layer_name="", srs_wkt=None
    ):
        super().__init__()

        from osgeo import gdal, ogr

        if dataset is not None:
            assert isinstance(dataset, gdal.Dataset) or isinstance(
                dataset, ogr.DataSource
            )
            self.ds = dataset
        elif isinstance(filename, (str, os.PathLike)):

            from osgeo_utils.auxiliary.util import GetOutputDriverFor

            if driver is None:
                driver = GetOutputDriverFor(filename, is_raster=False)

            ogr_drv = ogr.GetDriverByName(driver)
            self.ds = ogr_drv.CreateDataSource(str(filename))
        else:
            raise ValueError("Unhandled output type.")

        self.layer_name = layer_name
        self.prototype = {"type": "Feature", "properties": {}}
        self.srs_wkt = srs_wkt
        self.lyr = None

    def add_operation(self, op):
        # Create a prototype feature so that field names
        # match the order they are specified in input
        # operations.
        self.prototype["properties"][op.name] = None

    def add_column(self, col_name):
        self.prototype["properties"][col_name] = None

    def write(self, feature):
        from osgeo import ogr, osr

        if self.lyr is None:
            f = JSONFeature(copy.deepcopy(self.prototype))
            feature.copy_to(f)

            fields = self._collect_fields(f)

            srs = osr.SpatialReference()
            if self.srs_wkt:
                srs.ImportFromWkt(self.srs_wkt)

            self.lyr = self.ds.CreateLayer(self.layer_name, srs)
            for field_def in fields.values():
                self.lyr.CreateField(field_def)

        ogr_feature = ogr.Feature(self.lyr.GetLayerDefn())
        feature.copy_to(GDALFeature(ogr_feature))
        self.lyr.CreateFeature(ogr_feature)

    def features(self):
        return None

    @staticmethod
    def _collect_fields(feature):
        import numpy as np
        from osgeo import ogr

        ogr_fields = {}

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
                elif type(value) is np.ndarray:
                    if value.dtype == np.int64:
                        field_type = ogr.OFTInteger64List
                    elif value.dtype == np.int32:
                        field_type = ogr.OFTIntegerList
                    elif value.dtype == np.float64:
                        field_type = ogr.OFTRealList

                ogr_fields[field_name] = ogr.FieldDefn(field_name, field_type)

        return ogr_fields
