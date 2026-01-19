from typing import List, Optional

from ._exactextract import FeatureSequentialProcessor as _FeatureSequentialProcessor
from ._exactextract import Processor  # noqa: F401
from ._exactextract import RasterParallelProcessor as _RasterParallelProcessor
from ._exactextract import RasterSequentialProcessor as _RasterSequentialProcessor
from .feature import FeatureSource
from .operation import Operation
from .writer import Writer

__all__ = ["FeatureSequentialProcessor", "RasterParallelProcessor", "RasterSequentialProcessor"]


class FeatureSequentialProcessor(_FeatureSequentialProcessor):
    """Binding class around exactextract FeatureSequentialProcessor"""

    def __init__(
        self,
        ds: FeatureSource,
        writer: Writer,
        op_list: List[Operation],
        include_cols: Optional[List[Operation]] = None,
    ):
        """
        Args:
            ds (FeatureSource): Dataset to use
            writer (Writer): Writer to use
            op_list: List of Operations to perform
            include_cols: List of columns to copy from
               input features
        """
        super().__init__(ds, writer)
        for col in include_cols or []:
            self.add_col(col)
        for op in op_list:
            self.add_operation(op)


class RasterSequentialProcessor(_RasterSequentialProcessor):
    """Binding class around exactextract RasterSequentialProcessor"""

    def __init__(
        self,
        ds: FeatureSource,
        writer: Writer,
        op_list: List[Operation],
        include_cols: Optional[List[Operation]] = None,
    ):
        """
        Args:
            ds (FeatureSource): Dataset to use
            writer (Writer): Writer to use
            op_list (List[Operation]): List of operations
            include_cols: List of columns to copy from
                input features
        """
        super().__init__(ds, writer)
        for col in include_cols or []:
            self.add_col(col)
        for op in op_list:
            self.add_operation(op)


class RasterParallelProcessor(_RasterParallelProcessor):
    """Binding class around exactextract RasterParallelProcessor"""

    def __init__(
        self,
        ds: FeatureSource,
        writer: Writer,
        op_list: List[Operation],
        include_cols: Optional[List[Operation]] = None,
        threads: int = 4,
    ):
        """
        Args:
            ds (FeatureSource): Dataset to use
            writer (Writer): Writer to use
            op_list (List[Operation]): List of operations
            include_cols: List of columns to copy from
                input features
            threads (int): Number of threads for parallel processing
        """
        super().__init__(ds, writer, threads)
        for col in include_cols or []:
            self.add_col(col)
        for op in op_list:
            self.add_operation(op)
