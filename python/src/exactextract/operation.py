#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from __future__ import annotations

from typing import Optional

from _exactextract import StatDescriptor, Operation as _Operation

from .raster import GDALRasterWrapper


class Operation(_Operation):
    """ Binding class around exactextract GDALRasterWrapper """

    def __init__(self,
                 stat_name: str,
                 field_name: str,
                 raster: GDALRasterWrapper,
                 weights: Optional[GDALRasterWrapper] = None):
        """
        Create Operation object from stat name, field name, raster, and weighted raster

        Args:
            stat_name (str): Name of the stat. Refer to docs for options.
            field_name (str): Field name to use. Output of operation will have title \'{field_name}_{stat_name}\'
            raster (GDALRasterWrapper): Raster to compute over.
            weights (Optional[GDALRasterWrapper], optional): Weight raster to use. Defaults to None.
        """
        super().__init__(stat_name, field_name, raster, weights)

    @classmethod
    def from_descriptor(
            cls,
            desc: str,
            raster: GDALRasterWrapper,
            weights: Optional[GDALRasterWrapper] = None) -> Operation:
        """
        Create Operation object from exactextract stat descriptor (like what the CLI takes)

        Args:
            desc (str): exactextract stat descriptor (example: 'mean(temp)')
            raster (GDALRasterWrapper): exactextract GDALRasterWrapper to compute the stat over
            weights (Optional[GDALRasterWrapper], optional): Optional exactextract GDALRasterWrapper for weightrs. Defaults to None.

        Returns:
            Operation: exactextract Operation object
        """
        stat_desc = StatDescriptor.from_descriptor(desc)
        return cls(stat_desc.stat, stat_desc.name, raster, weights)