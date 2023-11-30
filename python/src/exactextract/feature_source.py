from _exactextract import FeatureSource

from .feature import GDALFeature, JSONFeature


class GDALFeatureSource(FeatureSource):
    def __init__(self, src, id_field):
        super().__init__(id_field)
        self.src = src

    def __iter__(self):
        for f in self.src:
            yield GDALFeature(f)


class JSONFeatureSource(FeatureSource):
    def __init__(self, src, id_field):
        super().__init__(id_field)
        if type(src) is dict:
            self.src = [src]
        else:
            self.src = src

    def __iter__(self):
        for f in self.src:
            yield JSONFeature(f)
