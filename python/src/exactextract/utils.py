#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from typing import List, Union

from osgeo import gdal, ogr


def get_ds_path(ds: Union[gdal.Dataset, ogr.DataSource]) -> str:
    """
    Get the file path of an OSGeo dataset

    Args:
        ds (Union[gdal.Dataset, ogr.DataSource]): Input OSGeo dataset

    Returns:
        str: File path to dataset
    """
    if isinstance(ds, gdal.Dataset):
        return ds.GetFileList()[0]     # type: ignore
    else:
        return ds.GetName()


def get_field_names(layer: ogr.Layer) -> List[str]:
    """
    Retrieves the names of the fields for a given layer.

    Args:
        layer (ogr.Layer): OGR layer object

    Returns:
        List[str]: list of field names
    """
    layer_defn = layer.GetLayerDefn()
    return [
        layer_defn.GetFieldDefn(idx).GetName()
        for idx in range(layer_defn.GetFieldCount())
    ]