import os

from .feature_source import (
    FeatureSource,
    GDALFeatureSource,
    JSONFeatureSource,
    GeoPandasFeatureSource,
)
from .raster_source import RasterSource, GDALRasterSource, RasterioRasterSource, XArrayRasterSource
from .operation import Operation
from .processor import FeatureSequentialProcessor, RasterSequentialProcessor
from .writer import JSONWriter


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
        import rioxarray
        import xarray

        if isinstance(rast, xarray.core.dataarray.DataArray):
            if band:
                return [XArrayRasterSource(rast, band)]
            else:
                if not names:
                    names = [f"{name_root}_{i+1}" for i in range(rast.rio.count)]
                return [
                        XArrayRasterSource(rast, i+1, name=names[i])
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
            vec = fiona.Open(vec)

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

    ops = []

    if weights_rast is None:
        weights_rast = [None]

    full_names = len(value_rast) > 1 or len(weights_rast) > 1

    def make_name(v, w, stat):
        if not full_names:
            return stat

        if stat.startswith("weighted"):
            if w:
                return f"{stat}_{v.name()}_{w.name()}"
            else:
                raise ValueError(f"No weights specified for {stat}")

        return f"{stat}_{v.name()}"

    if len(weights_rast) == len(value_rast):
        for v, w in zip(value_rast, weights_rast):
            ops += [Operation(stat, make_name(v, w, stat), v, w) for stat in stats]
    elif len(value_rast) > 1 and len(weights_rast) == 1:
        # recycle weights
        w = weights_rast[0]
        for v in value_rast:
            ops += [Operation(stat, make_name(v, w, stat), v, w) for stat in stats]
    elif len(value_rast) == 1 and len(weights_rast) > 1:
        # recycle values
        v = value_rast[0]
        for w in weights_rast:
            ops += [Operation(stat, make_name(v, w, stat), v, w) for stat in stats]
    else:
        raise ValueError

    # TODO dedup ops (non-weighted ops included with weighted)

    return ops


def prep_processor(strategy):
    processors = {
        "feature-sequential": FeatureSequentialProcessor,
        "raster-sequential": RasterSequentialProcessor,
    }

    return processors[strategy]


def exact_extract(
    rast,
    vec,
    ops,
    *,
    weights=None,
    include_cols=None,
    strategy="feature-sequential",
    max_cells_in_memory=30000000,
):
    rast = prep_raster(rast, name_root="band")
    weights = prep_raster(weights, name_root="weight")
    vec = prep_vec(vec)
    # TODO: check CRS and transform if necessary/possible?

    ops = prep_ops(ops, rast, weights)

    Processor = prep_processor(strategy)

    writer = JSONWriter()

    processor = Processor(vec, writer, ops)
    processor.set_max_cells_in_memory(max_cells_in_memory)
    if include_cols:
        for col in include_cols:
            processor.add_col(col)
    processor.process()

    return writer.features()
