import os

from ._exactextract import FeatureSource
from .feature import GDALFeature, JSONFeature


class GDALFeatureSource(FeatureSource):
    """
    A FeatureSource using the GDAL/OGR Python interface to provide features.
    """

    def __init__(self, src):
        """
        Create a GDALFeatureSource.

        Args:
            src: one of the following:
                 - string or Path to a file/datasource that can be opened with GDAL/OGR
                 - a `gdal.Dataset`
                 - an `ogr.DataSource`
                 - an `ogr.Layer`

            If the file has more than one layer, e.g., a GeoPackage, an `ogr.Layer` must be provided
            directly.
        """
        super().__init__()

        if isinstance(src, (str, os.PathLike)):
            from osgeo import ogr

            src = ogr.Open(src)

        try:
            nlayers = src.GetLayerCount()
        except AttributeError:
            self.src = src
        else:
            self.ds = src  # keep a reference to ds
            if nlayers > 1:
                raise Exception(
                    "Can only process a single layer; call directly with ogr.Layer object."
                )

            self.src = src.GetLayer(0)

    def __iter__(self):
        for f in self.src:
            yield GDALFeature(f)

    def srs_wkt(self):
        srs = self.src.GetSpatialRef()
        if srs:
            return srs.ExportToWkt()


class JSONFeatureSource(FeatureSource):
    """
    A FeatureSource providing GeoJSON-like features.
    """

    def __init__(self, src, *, srs_wkt=None):
        super().__init__()
        if type(src) is dict:
            self.src = [src]
        else:
            self.src = src

        self.srs_wkt_txt = srs_wkt

    def __iter__(self):
        for f in self.src:
            yield JSONFeature(f)

    def srs_wkt(self):
        return self.srs_wkt_txt


class GeoPandasFeatureSource(FeatureSource):
    """
    A FeatureSource using GeoPandas GeoDataFrame to provide features.
    """

    def __init__(self, src):
        super().__init__()
        self.src = src

    def __iter__(self):
        for f in self.src.iterfeatures():
            yield JSONFeature(f)

    def srs_wkt(self):
        return self.src.crs.to_wkt()
