import os

from .feature_source import (
    FeatureSource,
    GDALFeatureSource,
    GeoPandasFeatureSource,
    JSONFeatureSource,
)
from .operation import prepare_operations
from .processor import FeatureSequentialProcessor, RasterSequentialProcessor
from .raster_source import (
    GDALRasterSource,
    RasterioRasterSource,
    RasterSource,
    XArrayRasterSource,
)
from .writer import GDALWriter, JSONWriter, PandasWriter, Writer

__all__ = ["exact_extract"]


def prep_raster(rast, band=None, name_root=None, names=None):
    # TODO add some hooks to allow RasterSource implementations
    # defined outside this library to handle other input types.
    if rast is None:
        return [None]

    if isinstance(rast, RasterSource):
        return [rast]

    if hasattr(rast, "__iter__") and all(isinstance(x, RasterSource) for x in rast):
        return rast

    try:
        from osgeo import gdal

        if isinstance(rast, (str, os.PathLike)):
            rast = gdal.Open(rast)

        if isinstance(rast, gdal.Dataset):
            if band:
                return [GDALRasterSource(rast, band)]
            else:
                if not names:
                    names = [f"{name_root}_{i+1}" for i in range(rast.RasterCount)]
                return [
                    GDALRasterSource(rast, i + 1, name=names[i])
                    for i in range(rast.RasterCount)
                ]
    except ImportError:
        pass

    try:
        import rasterio

        if isinstance(rast, (str, os.PathLike)):
            rast = rasterio.open(rast)

        if isinstance(rast, rasterio.DatasetReader):
            if band:
                return [RasterioRasterSource(rast, band)]
            else:
                if not names:
                    names = [f"{name_root}_{i+1}" for i in range(rast.count)]
                return [
                    RasterioRasterSource(rast, i + 1, name=names[i])
                    for i in range(rast.count)
                ]
    except ImportError:
        pass

    try:
        import rioxarray  # noqa: F401
        import xarray

        if isinstance(rast, xarray.core.dataarray.DataArray):
            if band:
                return [XArrayRasterSource(rast, band)]
            else:
                if not names:
                    names = [f"{name_root}_{i+1}" for i in range(rast.rio.count)]
                return [
                    XArrayRasterSource(rast, i + 1, name=names[i])
                    for i in range(rast.rio.count)
                ]

    except ImportError:
        pass

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
            vec = ogr.Open(vec)

        if isinstance(vec, gdal.Dataset) or isinstance(vec, ogr.DataSource):
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

    raise Exception("Unhandled feature datatype")


def prep_ops(stats, values, weights=None):
    if type(stats) is str:
        stats = [stats]

    if weights is None:
        weights = []

    return prepare_operations(stats, values, weights)


def prep_processor(strategy):
    processors = {
        "feature-sequential": FeatureSequentialProcessor,
        "raster-sequential": RasterSequentialProcessor,
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
    elif output == "gdal":
        return GDALWriter(srs_wkt=srs_wkt, **options)

    raise Exception("Unsupported value of output")


def exact_extract(
    rast,
    vec,
    ops,
    *,
    weights=None,
    include_cols=None,
    include_geom=False,
    strategy="feature-sequential",
    max_cells_in_memory=30000000,
    output="geojson",
    output_options=None,
):
    """Calculate zonal statistics

    Args:
       rast: A `RasterSource` or filename that can be opened
             by GDAL/rasterio/xarray.
       vec: A `FeatureSource` or filename that can be opened
             by GDAL/fiona
       ops: A list of `Operation` objects, or strings that
            can be used to construct them
       weights: An optional `RasterSource` or filename for
            weights to be used in weighted operations.
       include_cols: An optional list of columns from the
            input features to be included into the output.
       include_geom: Flag indicating whether the geometry
            should be copied from the input features into
            the output.
       strategy: xx
       max_cells_in_memory=3000
       output: format of
       output_options:

    """
    rast = prep_raster(rast, name_root="band")
    weights = prep_raster(weights, name_root="weight")
    vec = prep_vec(vec)
    # TODO: check CRS and transform if necessary/possible?

    ops = prep_ops(ops, rast, weights)

    if type(include_cols) is str:
        include_cols = [include_cols]

    Processor = prep_processor(strategy)

    writer = prep_writer(output, vec.srs_wkt(), output_options)

    processor = Processor(vec, writer, ops, include_cols)
    if include_geom:
        processor.add_geom()
    processor.set_max_cells_in_memory(max_cells_in_memory)
    processor.process()

    writer.finish()

    return writer.features()
