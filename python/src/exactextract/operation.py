"""Operations for summarizing raster values."""

from __future__ import annotations

from typing import TYPE_CHECKING, Callable, Optional

from ._exactextract import Operation as _Operation
from ._exactextract import PythonOperation as _PythonOperation
from ._exactextract import (
    change_stat,  # noqa: F401
    prepare_operations,  # noqa: F401
)

if TYPE_CHECKING:
    from collections.abc import Mapping

    from .raster import RasterSource

__all__ = ["Operation", "PythonOperation"]


class Operation(_Operation):
    """Summarize of pixel values using a built-in function.

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
        """Constructor for Operation.

        Args:
            stat_name: Name of the stat. Refer to docs for options.
            field_name: Name of the result field that is assigned by this Operation.
            raster: Raster to compute over.
            weights: Weight raster to use. Defaults to None.
            options: Arguments used to control the behavior of an Operation, e.g.
                ``options={"q": 0.667}`` with ``stat_name = "quantile"``
        """
        if raster is None:
            raise TypeError

        if options is None:
            options = {}

        args = {str(k): str(v) for k, v in options.items()}

        super().__init__(stat_name, field_name, raster, weights, args)


class PythonOperation(_PythonOperation):
    """Summarize of pixel values using a Python function.

    Defines a summary operation to be performed on pixel values intersecting
    a geometry.
    """

    def __init__(
        self,
        function: Callable,
        field_name: str,
        raster: RasterSource,
        weights: Optional[RasterSource],
    ):
        """Constructor for PythonOperation.

        Args:
            function: Function accepting either two arguments (if `weights` is `None`),
                or three arguments. The function will be called with
                arrays of equal length containing:

                  - pixel values from `raster` (masked array)
                  - cell coverage fractions
                  - pixel values from `weights` (masked array)
            field_name: Name of the result field that is assigned by this Operation.
            raster: Raster to compute over.
            weights: Weight raster to use. Defaults to None.
        """
        if raster is None:
            raise TypeError

        # Inspect the function to determine if it should be called with
        # or without weights. This allows us to pass weights even if
        # they are unused, which is important to have weighted and
        # unweighted stats using the same common grid.
        weighted = function.__code__.co_argcount == 3

        super().__init__(function, field_name, raster, weights, weighted)
