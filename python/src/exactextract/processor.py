#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from typing import List

from _exactextract import FeatureSequentialProcessor as _FeatureSequentialProcessor
from _exactextract import RasterSequentialProcessor as _RasterSequentialProcessor

from .feature_source import FeatureSource
from .operation import Operation
from .writer import Writer


class FeatureSequentialProcessor(_FeatureSequentialProcessor):
    """Binding class around exactextract FeatureSequentialProcessor"""

    def __init__(self, ds: FeatureSource, writer: Writer, op_list: List[Operation]):
        """
        Create FeatureSequentialProcessor object

        Args:
            ds (FeatureSource): Dataset to use
            writer (Writer): Writer to use
            op_list (List[Operation]): List of operations
        """
        super().__init__(ds, writer)
        for op in op_list:
            self.add_operation(op)


class RasterSequentialProcessor(_RasterSequentialProcessor):
    """Binding class around exactextract RasterSequentialProcessor"""

    def __init__(self, ds: FeatureSource, writer: Writer, op_list: List[Operation]):
        """
        Create RasterSequentialProcessor object

        Args:
            ds (FeatureSource): Dataset to use
            writer (Writer): Writer to use
            op_list (List[Operation]): List of operations
        """
        super().__init__(ds, writer)
        for op in op_list:
            self.add_operation(op)
