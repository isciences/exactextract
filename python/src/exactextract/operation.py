#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from __future__ import annotations

from typing import Optional

from _exactextract import Operation as _Operation
from _exactextract import prepare_operations  # noqa: F401

from .raster_source import RasterSource


class Operation(_Operation):
    """Binding class around exactextract Operation"""

    def __init__(
        self,
        stat_name: str,
        field_name: str,
        raster: RasterSource,
        weights: Optional[RasterSource] = None,
    ):
        """
        Create Operation object from stat name, field name, raster, and weighted raster

        Args:
            stat_name (str): Name of the stat. Refer to docs for options.
            field_name (str): Field name to use. Output of operation will have title \'{field_name}_{stat_name}\'
            raster (RasterSource): Raster to compute over.
            weights (Optional[RasterSource], optional): Weight raster to use. Defaults to None.
        """
        if raster is None:
            raise TypeError
        super().__init__(stat_name, field_name, raster, weights)
