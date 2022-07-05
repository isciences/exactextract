#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pathlib
import pytest

from exactextract import (Operation, GDALRasterWrapper, GDALDatasetWrapper,
                          FeatureSequentialProcessor, MapWriter)

# Path to data
data_path = (pathlib.Path(__file__).parent / 'data').resolve()
simple_gpkg_path = pathlib.Path(data_path, 'simple_multilayer.gpkg')
assert simple_gpkg_path.is_file()
simple_tif_path = pathlib.Path(data_path, 'simple_raster.tif')
assert simple_tif_path.is_file()


class TestOperation():

    def test_valid_stat(self):
        rw = GDALRasterWrapper(simple_tif_path)
        dsw = GDALDatasetWrapper(simple_gpkg_path)
        writer = MapWriter()
        valid_stats = ('count', 'sum', 'mean', 'weighted_sum', 'weighted_mean',
                       'min', 'max', 'minority', 'majority', 'variety',
                       'variance', 'stdev', 'coefficient_of_variation')
        for stat in valid_stats:
            op = Operation(stat, 'test', rw)
            assert op.stat == stat
            processor = FeatureSequentialProcessor(dsw, writer, [op])
            processor.process()
            assert len(writer.output)
            assert all(len(v) for v in writer.output.values())
            writer.reset_operation(
            )     # This clears writer's ref to op when it goes out of scope
            writer.clear()

    def test_invalid_stat(self):
        rw = GDALRasterWrapper(simple_tif_path)
        dsw = GDALDatasetWrapper(simple_gpkg_path)
        writer = MapWriter()
        op = Operation('invalid_stat', 'test', rw)
        assert op.stat == 'invalid_stat'
        assert not len(writer.output)
        processor = FeatureSequentialProcessor(dsw, writer, [op])
        with pytest.raises(RuntimeError):
            processor.process()

    def test_field_name(self):
        rw = GDALRasterWrapper(simple_tif_path)
        op = Operation('count', 'any_field_name', rw)
        assert op.name == 'any_field_name'

    def test_valid_raster(self):
        rw = GDALRasterWrapper(simple_tif_path)
        op = Operation('count', 'test', rw)
        assert op.values == rw

    def test_invalid_raster(self):
        with pytest.raises(TypeError):
            Operation('count', 'test', 'invalid')     # type: ignore

    def test_valid_weights(self):
        rw = GDALRasterWrapper(simple_tif_path)
        rw2 = GDALRasterWrapper(simple_tif_path)
        op = Operation('count', 'test', rw, None)
        assert op.values == rw
        assert op.weights is None
        op = Operation('count', 'test', rw, rw2)
        assert op.values == rw
        assert op.weights == rw2

    def test_invalid_weights(self):
        rw = GDALRasterWrapper(simple_tif_path)
        with pytest.raises(TypeError):
            Operation('count', 'test', rw, 'invalid')     # type: ignore