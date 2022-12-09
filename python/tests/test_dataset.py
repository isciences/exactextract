#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pathlib
import pytest

from osgeo import gdal, ogr

from exactextract import GDALDatasetWrapper
from _exactextract import parse_dataset_descriptor

# Path to data
data_path = (pathlib.Path(__file__).parent / 'data').resolve()
simple_gpkg_path = pathlib.Path(data_path, 'simple_multilayer.gpkg')
assert simple_gpkg_path.is_file()


class TestGDALDatasetWrapper():

    def test_default_filename(self):
        dsw = GDALDatasetWrapper(str(simple_gpkg_path))
        assert dsw is not None

    def test_default_ds(self):
        for ds_fn in (gdal.OpenEx, ogr.Open):
            ds = ds_fn(str(simple_gpkg_path))
            dsw = GDALDatasetWrapper(ds)
            assert dsw is not None
            dsw = None
            ds = None

    def test_custom_layer(self):
        dsw = GDALDatasetWrapper(str(simple_gpkg_path), layer_idx=1)
        assert dsw is not None
        dsw = None

        dsw = GDALDatasetWrapper(str(simple_gpkg_path),
                                 layer_name='us_urban_areas')
        assert dsw is not None
        dsw = None

    def test_invalid_layer(self):
        with pytest.raises(RuntimeError):
            GDALDatasetWrapper(str(simple_gpkg_path), layer_idx=2)
        with pytest.raises(RuntimeError):
            GDALDatasetWrapper(str(simple_gpkg_path), layer_name='bad_input')

    def test_custom_field(self):
        dsw = GDALDatasetWrapper(str(simple_gpkg_path), field_idx=1)
        assert dsw is not None
        dsw = None

        dsw = GDALDatasetWrapper(str(simple_gpkg_path), field_name='NAME')
        assert dsw is not None
        dsw = None

    def test_invalid_field(self):
        with pytest.raises(IndexError):
            GDALDatasetWrapper(str(simple_gpkg_path), field_idx=2)
        with pytest.raises(ValueError):
            GDALDatasetWrapper(str(simple_gpkg_path), field_name='bad_input')

    def test_descriptor(self):
        assert parse_dataset_descriptor('test.gpkg') == ('test.gpkg', '0')
        assert parse_dataset_descriptor('test.gpkg[layer_name]') == (
            'test.gpkg', 'layer_name')

    def test_valid_descriptor(self):
        dsw = GDALDatasetWrapper.from_descriptor(
            str(simple_gpkg_path) + '[us_urban_areas]')
        assert dsw is not None

    def test_invalid_descriptor(self):
        # Layer name not given, and '0' is not a valid layer
        with pytest.raises(RuntimeError):
            GDALDatasetWrapper.from_descriptor(str(simple_gpkg_path))
        with pytest.raises(RuntimeError):
            GDALDatasetWrapper.from_descriptor(
                str(simple_gpkg_path) + '[bad_input]')

    def test_vfs(self):
        with pytest.raises(RuntimeError) as e:
            GDALDatasetWrapper('/vsimem/test')
        assert str(e.value) == "Failed to open dataset!"
