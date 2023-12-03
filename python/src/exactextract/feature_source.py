from _exactextract import FeatureSource

from .feature import GDALFeature, JSONFeature


class GDALFeatureSource(FeatureSource):
    def __init__(self, src):
        super().__init__("")
        try:
            nlayers = src.GetLayerCount()
        except AttributeError:
            self.src = src
        else:
            if nlayers > 1:
                raise Exception(
                    "Can only process a single layer; call directly with ogr.Layer object."
                )

            self.src = src.GetLayer(0)

    def __iter__(self):
        for f in self.src:
            yield GDALFeature(f)


class JSONFeatureSource(FeatureSource):
    def __init__(self, src):
        super().__init__("")
        if type(src) is dict:
            self.src = [src]
        else:
            self.src = src

    def __iter__(self):
        for f in self.src:
            yield JSONFeature(f)


class GeoPandasFeatureSource(FeatureSource):
    def __init__(self, src):
        super().__init__("")
        self.src = src

    def __iter__(self):
        for f in self.src.iterfeatures():
            yield JSONFeature(f)
