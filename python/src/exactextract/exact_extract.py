import copy
import functools
import os
import warnings
from itertools import chain
from typing import Mapping, Optional

from .feature import (
    FeatureSource,
    GDALFeatureSource,
    GeoPandasFeatureSource,
    JSONFeatureSource,
    QGISFeatureSource,
)
from .operation import Operation, PythonOperation, change_stat, prepare_operations
from .processor import FeatureSequentialProcessor, RasterParallelProcessor, RasterSequentialProcessor
from .raster import (
    GDALRasterSource,
    RasterioRasterSource,
    RasterSource,
    XArrayRasterSource,
)
from .writer import GDALWriter, JSONWriter, PandasWriter, QGISWriter, Writer

__all__ = ["exact_extract"]


def make_raster_names(root: str, nbands: int) -> list:
    if root:
        if nbands > 1:
            return [f"{root}_band_{i + 1}" for i in range(nbands)]
        else:
            return [f"{root}"]
    else:
        if nbands > 1:
            return [f"band_{i + 1}" for i in range(nbands)]
        else:
            return [""]


def prep_raster_gdal(rast, name_root=None) -> list:
    try:
        # eagerly import gdal_array to avoid possible ImportError when reading raster data
        from osgeo import gdal, gdal_array  # noqa: F401

        if isinstance(rast, (str, os.PathLike)):
            rast = gdal.Open(str(rast))

        if isinstance(rast, gdal.Dataset):
            # Handle inputs such as a netCDF with multiple variables
            if rast.RasterCount == 0 and rast.GetSubDatasets():
                sources = []
                for subds, _ in rast.GetSubDatasets():
                    try:
                        varname = gdal.GetSubdatasetInfo(subds).GetSubdatasetComponent()
                    except AttributeError:
                        varname = subds.split(":")[-1]
                    sources.append(prep_raster_gdal(subds, name_root=varname))
                return list(chain.from_iterable(sources))

            names = make_raster_names(name_root, rast.RasterCount)
            return [
                GDALRasterSource(rast, i + 1, name=names[i])
                for i in range(rast.RasterCount)
            ]
    except ImportError:
        pass


def prep_raster_rasterio(rast, name_root=None) -> list:
    try:
        import rasterio

        if isinstance(rast, (str, os.PathLike)):
            rast = rasterio.open(rast)

        if isinstance(rast, rasterio.DatasetReader):
            # Handle inputs such as a netCDF with multiple variables
            if rast.count == 0 and rast.subdatasets:
                sources = []
                for subds in rast.subdatasets:
                    varname = subds.split(":")[-1]
                    sources.append(prep_raster_rasterio(subds, name_root=varname))
                return list(chain.from_iterable(sources))

            names = make_raster_names(name_root, rast.count)
            return [
                RasterioRasterSource(rast, i + 1, name=names[i])
                for i in range(rast.count)
            ]
    except ImportError:
        pass


def prep_raster_xarray(rast, name_root=None) -> list:

    try:
        import rioxarray  # noqa: F401
        import xarray

        if isinstance(rast, xarray.core.dataset.Dataset):
            sources = [
                prep_raster_xarray(rast[varname], name_root=varname)
                for varname in rast.data_vars.keys()
            ]

            return list(chain.from_iterable(sources))

        if isinstance(rast, xarray.core.dataarray.DataArray):
            names = make_raster_names(name_root, rast.rio.count)
            return [
                XArrayRasterSource(rast, i + 1, name=names[i])
                for i in range(rast.rio.count)
            ]

    except ImportError:
        pass


def prep_raster(rast, name_root=None) -> list:
    # TODO add some hooks to allow RasterSource implementations
    # defined outside this library to handle other input types.
    if rast is None:
        return [None]

    if isinstance(rast, RasterSource):
        return [rast]

    if type(rast) in (list, tuple):
        if all(isinstance(src, (str, os.PathLike)) for src in rast):
            sources = [
                prep_raster(src, name_root=os.path.splitext(os.path.basename(src))[0])
                for src in rast
            ]
        else:
            sources = [prep_raster(src) for src in rast]
        return list(chain.from_iterable(sources))

    for loader in (prep_raster_gdal, prep_raster_rasterio, prep_raster_xarray):
        sources = loader(rast, name_root)
        if sources:
            return sources

    raise Exception("Unhandled raster datatype")


def prep_vec(vec):
    # TODO add some hooks to allow FeatureSource implementations
    # defined outside this library to handle other input types.
    if isinstance(vec, FeatureSource):
        return vec

    if type(vec) is list and len(vec) > 0 and "geometry" in vec[0]:
        return JSONFeatureSource(vec)

    if type(vec) is dict and "geometry" in vec:
        return JSONFeatureSource([vec])

    try:
        from osgeo import gdal, ogr

        if isinstance(vec, (str, os.PathLike)):
            vec = ogr.Open(str(vec))

        if isinstance(vec, (gdal.Dataset, ogr.DataSource, ogr.Layer)):
            return GDALFeatureSource(vec)
    except ImportError:
        pass

    try:
        import fiona

        if isinstance(vec, (str, os.PathLike)):
            vec = fiona.open(vec)

        if isinstance(vec, fiona.Collection):
            return JSONFeatureSource(vec)
    except ImportError:
        pass

    try:
        import geopandas

        if isinstance(vec, geopandas.GeoDataFrame):
            return GeoPandasFeatureSource(vec)
    except ImportError:
        pass

    try:
        import qgis.core

        if isinstance(vec, qgis.core.QgsVectorLayer):
            return QGISFeatureSource(vec)

    except ImportError:
        pass

    raise Exception("Unhandled feature datatype")


def prep_ops(stats, values, weights=None, *, add_unique=False):
    if type(stats) is str or isinstance(stats, Operation) or callable(stats):
        stats = [stats]

    if weights is None:
        weights = []

    ops = []

    for stat in stats:
        if type(stat) is str:
            ops += prepare_operations([stat], values, weights)
        elif callable(stat):
            # We want to hook into the value/weight looping and column-naming
            # functionality of prepare_operations, but we cannot create a
            # PythonOperation from inside prepare_operations. So we create a
            # set of dummy operations using a named stat and use properties of
            # these operations to create a set of PythonOperations.
            nargs = stat.__code__.co_argcount
            if nargs == 3:
                dummy_stat = "weighted_sum"
            elif nargs == 2:
                dummy_stat = "count"
            else:
                raise Exception(
                    f"Summary operation {stat.__name__} must take 2 or 3 arguments"
                )

            try:
                for op in prepare_operations([dummy_stat], values, weights):
                    ops.append(
                        PythonOperation(
                            stat,
                            op.name.replace(dummy_stat, stat.__name__),
                            op.values,
                            op.weights,
                        )
                    )
            except RuntimeError as e:
                if "No weights provided" in str(e):
                    raise Exception(
                        f"No weights provided but {stat.__name__} expects 3 arguments"
                    )
                else:
                    raise
        elif isinstance(stat, Operation):
            ops.append(stat)
        else:
            raise ValueError("Don't know how to handle stat", stat)

    if add_unique:
        need_unique = {}
        for op in ops:
            if op.stat in {"frac", "weighted_frac"}:
                need_unique[op.key()] = op
        for op in ops:
            if op.stat == "unique":
                del need_unique[op.key()]
        for op in need_unique.values():
            for unique_op in prepare_operations(
                [change_stat(op, "unique")], values, weights
            ):
                unique_op.name = "@delete@" + unique_op.name
                ops.append(unique_op)

    return ops


def prep_processor(strategy):
    processors = {
        "feature-sequential": FeatureSequentialProcessor,
        "raster-sequential": RasterSequentialProcessor,
        "raster-parallel": RasterParallelProcessor,
    }

    return processors[strategy]


def prep_writer(output, srs_wkt, options):
    if options is None:
        options = {}

    if isinstance(output, Writer):
        assert not options
        return output
    elif output == "geojson":
        return JSONWriter(**options)
    elif output == "pandas":
        return PandasWriter(srs_wkt=srs_wkt, **options)
    elif output == "qgis":
        return QGISWriter(srs_wkt=srs_wkt, **options)
    elif output == "gdal":
        return GDALWriter(srs_wkt=srs_wkt, **options)

    raise Exception("Unsupported value of output")


@functools.cache
def crs_matches(wkt_a, wkt_b):
    if wkt_a is None or wkt_b is None:
        return True

    if wkt_a == wkt_b:
        return True

    try:
        from osgeo import osr

        srs_a = osr.SpatialReference()
        srs_b = osr.SpatialReference()

        srs_a.ImportFromWkt(wkt_a)
        srs_b.ImportFromWkt(wkt_b)

        try:
            srs_a.StripVertical()
            srs_b.StripVertical()
        except AttributeError:
            pass

        return bool(srs_a.IsSame(srs_b))

    except ImportError:
        pass

    try:
        from pyproj import CRS

        crs_a = CRS.from_string(wkt_a)
        crs_b = CRS.from_string(wkt_b)

        return crs_a == crs_b

    except ImportError:
        pass

    return False


def warn_on_crs_mismatch(vec, ops):
    check_rast_vec = True
    check_rast_weights = True

    for op in ops:
        if check_rast_vec and not crs_matches(vec.srs_wkt(), op.values.srs_wkt()):
            check_rast_vec = False
            warnings.warn(
                "Spatial reference system of input features does not exactly match raster.",
                RuntimeWarning,
            )

        if (
            check_rast_weights
            and op.weights is not None
            and not crs_matches(vec.srs_wkt(), op.weights.srs_wkt())
        ):
            check_rast_weights = False
            warnings.warn(
                "Spatial reference system of input features does not exactly match weighting raster.",
                RuntimeWarning,
            )


def exact_extract(
    rast,
    vec,
    ops,
    *,
    weights=None,
    include_cols=None,
    include_geom=False,
    strategy: str = "feature-sequential",
    threads: int = 4,
    max_cells_in_memory: int = 30000000,
    grid_compat_tol: float = 1e-3,
    output: str = "geojson",
    output_options: Optional[Mapping] = None,
    progress=False,
):
    """Calculate zonal statistics.

    Args:
       rast: One or a list of :py:class:`RasterSource` object(s) or filename(s) that
             can be opened by GDAL/rasterio/xarray.
       vec: A :py:class:`FeatureSource` or filename that can be opened
             by GDAL/fiona
       ops: A list of :py:class:`Operation` objects, or strings that
            can be used to construct them (e.g., ``"mean"``, ``"quantile(q=0.33)"``).
            Check out :doc:`Available operations </operations>` for more information.
       weights: An optional :py:class:`RasterSource` or filename for
            weights to be used in weighted operations.
       include_cols: An optional list of columns from the
            input features to be included into the output.
       include_geom: Flag indicating whether the geometry
            should be copied from the input features into
            the output.
       strategy: Specifies the strategy to use when processing features.
          Detailed performance notes are available in
          :ref:`Performance - Processing strategies <performance-processing-strategies>`.
          Must be set to one of:

                 - ``"feature-sequential"`` (the default):
                   iterate over the features in ``vec``,
                   read the corresponding pixels from ``rast``/``weights``,
                   and compute the summary operations. This offers predictable
                   memory consumption but may be inefficient if the order of
                   features in ``vec`` causes the same, relatively large raster
                   blocks to be read and decompressed many times.

                 - ``"raster-sequential"``:
                   iterate over chunks of pixels in ``rast``, identify the intersecting
                   features from ``vec``, and compute statistics. This performs better
                   than ``strategy="feature-sequential"`` in some cases, but comes at
                   a cost of higher memory usage.

                 - ``"raster-parallel"``:
                   iterate over chunks of pixels in ``rast`` in parallel, identify the
                   intersecting features from ``vec``, and compute statistics. Uses
                   TBB for parallel processing. Does not support weighted operations
                   or variance operations (e.g., ``"variance"``, ``"std"``).
       threads: Number of threads to use for parallel processing when
                ``strategy="raster-parallel"``. Default is 4.
       max_cells_in_memory: Indicates the maximum number of raster cells that should be
                            loaded into memory at a given time.
       grid_compat_tol: require value and weight grids to align within ``grid_compat_tol`` times the smaller of the two grid resolutions
       output: An :py:class:`OutputWriter` or one of the following strings:

                 - "geojson" (the default): return a list of GeoJSON-like features
                 - "pandas": return a ``pandas.DataFrame`` or ``geopandas.GeoDataFrame``,
                             depending on the value of ``include_geom``
                 - "gdal": write results to disk using GDAL/OGR as they are written. This
                           option (with ``strategy="feature-sequential"``) avoids the need
                           to maintain results for all features in memory at a single time,
                           which may be significant for operations with large result sizes
                           such as ``cell_id``, ``values``, etc.
       output_options: an optional dictionary of options passed to the :py:class:`writer.JSONWriter`, :py:class:`writer.PandasWriter`, or :py:class:`writer.GDALWriter`.
       progress: if `True`, a progress bar will be displayed. Alternatively, a
                 function may be provided that will be called with the completion fraction
                 and a status message.
    """
    rast = prep_raster(rast)
    weights = prep_raster(weights, name_root="weight")
    vec = prep_vec(vec)

    if output_options is None:
        output_options = {}

    ops = prep_ops(
        ops, rast, weights, add_unique=output_options.get("frac_as_map", False)
    )

    warn_on_crs_mismatch(vec, ops)

    if "frac_as_map" in output_options:
        output_options = copy.copy(output_options)
        del output_options["frac_as_map"]
        output_options["map_fields"] = {
            "frac": ("unique", "frac"),
            "weighted_frac": ("unique", "weighted_frac"),
        }

    if type(include_cols) is str:
        include_cols = [include_cols]

    if strategy == "raster-parallel":
        for op in ops:
            if op.weighted():
                raise ValueError(
                    "Weighted operations are not supported with strategy='raster-parallel'. "
                    "Use strategy='feature-sequential' or strategy='raster-sequential' instead."
                )
            if op.requires_variance():
                raise ValueError(
                    "Variance operations are not supported with strategy='raster-parallel'. "
                    "Use strategy='feature-sequential' or strategy='raster-sequential' instead."
                )

    Processor = prep_processor(strategy)

    writer = prep_writer(output, vec.srs_wkt(), output_options)

    if strategy == "raster-parallel":
        processor = Processor(vec, writer, ops, include_cols, threads)
    else:
        processor = Processor(vec, writer, ops, include_cols)
    if include_geom:
        processor.add_geom()
    processor.set_max_cells_in_memory(max_cells_in_memory)
    processor.set_grid_compat_tol(grid_compat_tol)

    if progress:
        processor.show_progress(True)

        if callable(progress):
            processor.set_progress_fn(progress)
        elif progress is True:
            try:
                import tqdm

                bar = tqdm.tqdm(total=100)

                def status(frac, message):
                    pct = frac * 100
                    bar.update(pct - bar.n)
                    bar.set_description(message)
                    if pct == 100:
                        bar.close()

            except ImportError:

                def status(frac, message):
                    print(f"[{frac * 100:0.1f}%] {message}")

            processor.set_progress_fn(status)
        else:
            raise ValueError("progress should be True or a function")

    processor.process()
    writer.finish()

    return writer.features()
