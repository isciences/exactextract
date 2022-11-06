#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pathlib

from exactextract import (Operation, GDALRasterWrapper, GDALDatasetWrapper,
                          FeatureSequentialProcessor,
                          RasterSequentialProcessor, MapWriter)

# Path to data
data_path = (pathlib.Path(__file__).parent / 'data').resolve()
simple_gpkg_path = pathlib.Path(data_path, 'simple_multilayer.gpkg')
assert simple_gpkg_path.is_file()
simple_tif_path = pathlib.Path(data_path, 'simple_raster.tif')
assert simple_tif_path.is_file()


class TestFeatureSequentialProcessor():

    def test_process(self):
        rw = GDALRasterWrapper(simple_tif_path)
        rw2 = GDALRasterWrapper(simple_tif_path)
        dsw = GDALDatasetWrapper(simple_gpkg_path)
        writer = MapWriter()
        op = Operation('count', 'test', rw)
        op2 = Operation('sum', 'my_test', rw2)
        assert not len(writer.output)
        processor = FeatureSequentialProcessor(dsw, writer, [op, op2])
        processor.process()
        assert len(writer.output)
        assert all(len(v) == 2 for v in writer.output.values())

    def test_process_multiband_tif(self):
        # Test added to fix bug for using multiple operations on different TIF bands
        rw_0 = GDALRasterWrapper(simple_tif_path, band_idx=2)
        rw_1 = GDALRasterWrapper(simple_tif_path, band_idx=1)
        dsw = GDALDatasetWrapper(simple_gpkg_path)
        writer = MapWriter()
        op_0 = Operation('sum', 'test', rw_0)
        op_1 = Operation('sum', 'test2', rw_1)
        assert not len(writer.output)
        processor = FeatureSequentialProcessor(dsw, writer, [op_0, op_1])
        processor.process()
        assert len(writer.output)
        for v in writer.output.values():
            assert len(v) == 2
            assert v[0] != v[1]


class TestRasterSequentialProcessor():

    def test_process(self):
        rw = GDALRasterWrapper(simple_tif_path)
        rw2 = GDALRasterWrapper(simple_tif_path)
        dsw = GDALDatasetWrapper(simple_gpkg_path)
        writer = MapWriter()
        op = Operation('count', 'test', rw)
        op2 = Operation('sum', 'my_test', rw2)
        assert not len(writer.output)
        processor = RasterSequentialProcessor(dsw, writer, [op, op2])
        processor.process()
        assert len(writer.output)
        assert all(len(v) == 2 for v in writer.output.values())

    def test_process_multiband_tif(self):
        # Test added to fix bug for using multiple operations on different TIF bands
        rw_0 = GDALRasterWrapper(simple_tif_path, band_idx=2)
        rw_1 = GDALRasterWrapper(simple_tif_path, band_idx=1)
        dsw = GDALDatasetWrapper(simple_gpkg_path)
        writer = MapWriter()
        op_0 = Operation('sum', 'test', rw_0)
        op_1 = Operation('sum', 'test2', rw_1)
        assert not len(writer.output)
        processor = RasterSequentialProcessor(dsw, writer, [op_0, op_1])
        processor.process()
        assert len(writer.output)
        for v in writer.output.values():
            assert len(v) == 2
            assert v[0] != v[1]
