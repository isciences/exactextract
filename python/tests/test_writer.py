#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import pathlib
import subprocess
import tempfile
import pytest

from osgeo import gdal, ogr

from exactextract import (Operation, GDALRasterWrapper, GDALDatasetWrapper,
                          FeatureSequentialProcessor, MapWriter, GDALWriter)

# Path to data
data_path = (pathlib.Path(__file__).parent / 'data').resolve()
simple_gpkg_path = pathlib.Path(data_path, 'simple_multilayer.gpkg')
assert simple_gpkg_path.is_file()
simple_tif_path = pathlib.Path(data_path, 'simple_raster.tif')
assert simple_tif_path.is_file()


def run_in_new_process(shell_str):
    """
    Helper method to run code in another process.
    This way, if SIGSEGV is thrown we can catch the
    non-zero exit code

    Args:
        shell_str (str): Python code to execute

    Returns:
        int: Process return code after execution
    """
    this_path = pathlib.Path(__file__).parent.resolve()
    proc = subprocess.Popen('%s -c "%s"' % (sys.executable, shell_str),
                            shell=True,
                            stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL,
                            cwd=str(this_path))
    proc.communicate()
    return proc.returncode


class TestMapWriter():

    @staticmethod
    def process_writer(writer):
        rw = GDALRasterWrapper(simple_tif_path)
        dsw = GDALDatasetWrapper(simple_gpkg_path)
        op = Operation('count', 'test', rw)
        processor = FeatureSequentialProcessor(dsw, writer, [op])
        processor.process()

    def test_basic(self):
        writer = MapWriter()
        assert not len(writer.output)
        self.process_writer(writer)
        assert len(writer.output)
        assert all(len(v) for v in writer.output.values())

    def test_clear(self):
        writer = MapWriter()
        assert not len(writer.output)
        self.process_writer(writer)
        assert len(writer.output)
        assert all(len(v) for v in writer.output.values())
        writer.clear()
        assert not len(writer.output)

    @staticmethod
    def good_process():
        """ Test to ensure no segfault """
        writer = MapWriter()
        rw = GDALRasterWrapper(simple_tif_path)
        dsw = GDALDatasetWrapper(simple_gpkg_path)
        for stat in ('count', 'mean'):
            writer.reset_operation(
            )     # w/o this the second iter would segfault due to original op out of scope
            op = Operation(stat, 'test', rw)
            processor = FeatureSequentialProcessor(dsw, writer, [op])
            processor.process()

    def test_reset_operation(self):
        assert run_in_new_process('from test_writer import TestMapWriter;'
                                  'TestMapWriter.good_process()') == 0


class TestGDALWriter():

    def test_valid_file(self):
        dsw = GDALDatasetWrapper(simple_gpkg_path)
        with tempfile.TemporaryDirectory() as tmp_dir:
            path = pathlib.Path(tmp_dir) / 'output.csv'
            GDALWriter(path, dsw)
        GDALWriter('/vsimem/output.dbf', dsw)

    def test_invalid_file(self):
        dsw = GDALDatasetWrapper(simple_gpkg_path)
        with pytest.raises(RuntimeError):
            GDALWriter('/invalid_1234/output.dbf', dsw)
        with tempfile.TemporaryDirectory() as tmp_dir:
            path = pathlib.Path(tmp_dir) / 'output.fake'
            with pytest.raises(RuntimeError):
                GDALWriter(path, dsw)

    def test_valid_ds(self):
        dsw = GDALDatasetWrapper(simple_gpkg_path)
        tmp_path = '/vsimem/temp.csv'
        driver = ogr.GetDriverByName('CSV')
        ds = driver.CreateDataSource(tmp_path)
        GDALWriter(ds, dsw)
        gdal.Unlink(tmp_path)
        ds = None

    def test_invalid_ds(self):
        dsw = GDALDatasetWrapper(simple_gpkg_path)
        tmp_path = '/vsimem/temp.tif'
        driver = gdal.GetDriverByName('GTiff')
        ds = driver.Create(tmp_path, 50, 50, 1, gdal.GDT_Byte)
        with pytest.raises(RuntimeError):
            GDALWriter(ds, dsw)
        gdal.Unlink(tmp_path)
        ds = None

    def test_process(self):
        rw = GDALRasterWrapper(simple_tif_path)
        dsw = GDALDatasetWrapper(simple_gpkg_path)
        op = Operation('count', 'test', rw)
        with tempfile.TemporaryDirectory() as tmp_dir:
            path = pathlib.Path(tmp_dir) / 'output.csv'
            assert not path.exists()
            writer = GDALWriter(path, dsw)
            processor = FeatureSequentialProcessor(dsw, writer, [op])
            processor.process()
            writer = None     # note writer must be destroyed in order to flush to disk!
            assert path.exists()
            assert len(path.read_text())
