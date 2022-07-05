#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pathlib
from typing import Union, Dict, List

from osgeo import gdal, ogr

from _exactextract import MapWriter as _MapWriter, GDALWriter as _GDALWriter

from .dataset import GDALDatasetWrapper
from .utils import get_ds_path


class MapWriter(_MapWriter):
    """ Binding class around exactextract MapWriter """

    def __init__(self):
        super().__init__()

    @property
    def output(self) -> Dict[str, List[float]]:
        """
        Returns output map / dict from the writer

        Returns:
            Dict[str, List[float]]: Output map / dict
        """
        return self.get_map()


class GDALWriter(_GDALWriter):
    """ Binding class around exactextract MapWriter """

    def __init__(self, filename_or_ds: Union[str, pathlib.Path, gdal.Dataset,
                                             ogr.DataSource],
                 input_ds: GDALDatasetWrapper):
        """
        Create exactextract GDALWriter object from Python OSGeo Dataset
        object or from file path.

        Args:
            filename_or_ds (Union[str, pathlib.Path, gdal.Dataset, ogr.DataSource]): File path or OSGeo Dataset / DataSource
            input_ds (GDALDatasetWrapper): Reference dataset to work with. This will be used to get the correct field to write.

        Raises:
            RuntimeError: If the file path was not found
        """
        # Get file path based on input, filename or dataset
        if isinstance(filename_or_ds, (gdal.Dataset, ogr.DataSource)):
            path = pathlib.Path(get_ds_path(filename_or_ds))
        else:
            path = pathlib.Path(filename_or_ds)

        # Assert the parent directory and resolve the full path
        if not path.parent.is_dir() and not path.parent.name.startswith(
                'vsimem'):
            raise RuntimeError('Parent directory path not found: %s' %
                               str(path.parent))
        path = path.resolve()

        super().__init__(str(path))
        self.copy_id_field(input_ds)