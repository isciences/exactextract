#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from typing import List, Union

from _exactextract import (FeatureSequentialProcessor as
                           _FeatureSequentialProcessor,
                           RasterSequentialProcessor as
                           _RasterSequentialProcessor)

from .dataset import GDALDatasetWrapper
from .operation import Operation
from .writer import MapWriter, GDALWriter


class FeatureSequentialProcessor(_FeatureSequentialProcessor):
    """ Binding class around exactextract FeatureSequentialProcessor """

    def __init__(self, ds_wrapper: GDALDatasetWrapper,
                 writer: Union[MapWriter,
                               GDALWriter], op_list: List[Operation]):
        """
        Create FeatureSequentialProcessor object

        Args:
            ds_wrapper (GDALDatasetWrapper): Dataset to use
            writer (Union[MapWriter, GDALWriter]): Writer to use
            op_list (List[Operation]): List of operations
        """
        super().__init__(ds_wrapper, writer, op_list)


class RasterSequentialProcessor(_RasterSequentialProcessor):
    """ Binding class around exactextract RasterSequentialProcessor """

    def __init__(self, ds_wrapper: GDALDatasetWrapper,
                 writer: Union[MapWriter,
                               GDALWriter], op_list: List[Operation]):
        """
        Create RasterSequentialProcessor object

        Args:
            ds_wrapper (GDALDatasetWrapper): Dataset to use
            writer (Union[MapWriter, GDALWriter]): Writer to use
            op_list (List[Operation]): List of operations
        """
        super().__init__(ds_wrapper, writer, op_list)