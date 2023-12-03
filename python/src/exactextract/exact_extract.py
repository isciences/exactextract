from .feature_source import (
    FeatureSource,
    GDALFeatureSource,
    JSONFeatureSource,
    GeoPandasFeatureSource,
)
from .raster_source import RasterSource, GDALRasterSource, RasterioRasterSource
from .operation import Operation
from .processor import FeatureSequentialProcessor, RasterSequentialProcessor
from .writer import JSONWriter


def prep_raster(rast):
    # TODO add some hooks to allow RasterSource implementations
    # defined outside this library to handle other input types.
    if isinstance(rast, RasterSource):
        return rast

    try:
        from osgeo import gdal

        if isinstance(rast, gdal.Dataset):
            return GDALRasterSource(rast)
    except ImportError:
        pass

    try:
        import rasterio

        if isinstance(rast, rasterio.DatasetReader):
            return RasterioRasterSource(rast)
    except ImportError:
        pass

    raise Exception("Unhandled raster datatype")


def prep_vec(vec):
    # TODO add some hooks to allow FeatureSource implementations
    # defined outside this library to handle other input types.
    if isinstance(vec, FeatureSource):
        return vec

    try:
        from osgeo import gdal, ogr

        if isinstance(vec, gdal.Dataset) or isinstance(vec, ogr.DataSource):
            return GDALFeatureSource(vec)
    except ImportError:
        pass

    if type(vec) is list and len(vec) > 0 and "geometry" in vec[0]:
        return JSONFeatureSource(vec)

    try:
        import fiona

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


def prep_ops(stats, value_rast, weights_rast=None):
    if type(stats) is str:
        stats = [stats]

    # TODO derive names based on stat and value/weight names
    ops = [Operation(stat, stat, value_rast, weights_rast) for stat in stats]

    return ops


def prep_processor(strategy):
    processors = {
        "feature-sequential": FeatureSequentialProcessor,
        "raster-sequential": RasterSequentialProcessor,
    }

    return processors[strategy]


def exact_extract(rast, vec, ops, *, weights=None, include_cols=None, strategy="feature-sequential"):
    rast = prep_raster(rast)
    vec = prep_vec(vec)
    # TODO: check CRS and transform if necessary/possible?

    ops = prep_ops(ops, rast, weights)

    Processor = prep_processor(strategy)

    writer = JSONWriter()

    processor = Processor(vec, writer, ops)
    if include_cols:
        for col in include_cols:
            processor.add_col(col)
    processor.process()

    return writer.features()
