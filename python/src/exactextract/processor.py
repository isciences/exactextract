from typing import List, Optional

from ._exactextract import FeatureSequentialProcessor as _FeatureSequentialProcessor
from ._exactextract import Processor  # noqa: F401
from ._exactextract import RasterSequentialProcessor as _RasterSequentialProcessor
from .feature import FeatureSource
from .operation import Operation
from .writer import Writer


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
        Create FeatureSequentialProcessor object

        Args:
            ds (FeatureSource): Dataset to use
            writer (Writer): Writer to use
            op_list (List[Operation]): List of operations
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
        Create RasterSequentialProcessor object

        Args:
            ds (FeatureSource): Dataset to use
            writer (Writer): Writer to use
            op_list (List[Operation]): List of operations
        """
        super().__init__(ds, writer)
        for col in include_cols or []:
            self.add_col(col)
        for op in op_list:
            self.add_operation(op)
