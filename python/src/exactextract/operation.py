from __future__ import annotations

from typing import Mapping, Optional

from ._exactextract import Operation as _Operation
from ._exactextract import PythonOperation  # noqa: F401
from ._exactextract import prepare_operations  # noqa: F401
from .raster import RasterSource


class Operation(_Operation):
    """
    Summary of pixel values

    Defines a summary operation to be performed on pixel values intersecting
    a geometry. May return a scalar (e.g., ``weighted_mean``), or a
    vector (e.g., ``coverage``).
    """

    def __init__(
        self,
        stat_name: str,
        field_name: str,
        raster: RasterSource,
        weights: Optional[RasterSource] = None,
        options: Optional[Mapping] = None,
    ):
        """
        Create an Operation

        Args:
            stat_name (str): Name of the stat. Refer to docs for options.
            field_name (str): Name of the result field that is assigned by this Operation.
            raster (RasterSource): Raster to compute over.
            weights (Optional[RasterSource], optional): Weight raster to use. Defaults to None.
            options: Arguments used to control the behavior of an Operation, e.g. ``options={"q": 0.667}``
                     with ``stat_name = "quantile"``
        """
        if raster is None:
            raise TypeError

        if options is None:
            options = {}

        args = {str(k): str(v) for k, v in options.items()}

        super().__init__(stat_name, field_name, raster, weights, args)
