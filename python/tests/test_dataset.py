#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pathlib
import zipfile

import pytest
from osgeo import gdal, ogr

from exactextract import GDALDatasetWrapper
from _exactextract import parse_dataset_descriptor

# Path to data
data_path = (pathlib.Path(__file__).parent / 'data').resolve()
simple_gpkg_path = pathlib.Path(data_path, 'simple_multilayer.gpkg')
assert simple_gpkg_path.is_file()


# For virtual filesystem tests
@pytest.fixture(scope='session')
def zip_path(tmp_path_factory):
    # Zip our raster in a temp directory
    zip_path = tmp_path_factory.mktemp('data') / 'simple_raster.zip'
    with zipfile.ZipFile(zip_path, 'w') as zip_object:
        zip_object.write(simple_gpkg_path, simple_gpkg_path.name)
    return zip_path


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

    def test_valid_vfs(self, zip_path):
        assert zip_path.exists()

        # Load using /vsizip
        dsw = GDALDatasetWrapper(
            f'/vsizip/{zip_path.resolve()}/{simple_gpkg_path.name}')
        assert dsw is not None
        dsw = None

    def test_valid_vfs_as_ds(self, zip_path):
        assert zip_path.exists()
        for ds_fn in (gdal.OpenEx, ogr.Open):
            ds = ds_fn(f'/vsizip/{zip_path.resolve()}/{simple_gpkg_path.name}')
            assert ds is not None
            dsw = GDALDatasetWrapper(ds)
            assert dsw is not None
            dsw = None
            ds = None

    def test_vsi_inmem(self):
        vsi_str = '/vsimem/simple.gpkg'

        # Open GPKG file, and copy to inmem
        ds = ogr.Open(str(simple_gpkg_path))
        assert ds is not None
        layer = ds.GetLayer()

        # NOTE: We cannot use the 'Memory' OGR driver, but we can use vsimem with
        # another driver
        # From https://gdal.org/drivers/vector/memory.html#vector-memory:
        # There is no way to open an existing Memory datastore.
        # It must be created with CreateDataSource() and populated
        # and used from that handle.
        # When the datastore is closed all contents are freed and destroyed.
        driver = ogr.GetDriverByName('GPKG')
        ds_inmem = driver.CreateDataSource(vsi_str)
        assert ds_inmem is not None
        layer_inmem = ds_inmem.CreateLayer(
            layer.GetName(),
            srs=layer.GetSpatialRef(),
            geom_type=layer.GetLayerDefn().GetGeomType())
        feat = layer.GetFeature(1)
        [
            layer_inmem.CreateField(feat.GetFieldDefnRef(i))
            for i in range(feat.GetFieldCount())
        ]
        [
            layer_inmem.CreateFeature(layer.GetFeature(i + 1).Clone())
            for i in range(layer.GetFeatureCount())
        ]
        assert layer_inmem.GetFeatureCount() > 0

        # Load by dataset
        dsw = GDALDatasetWrapper(ds_inmem)
        assert dsw is not None
        dsw = None

        # Load by VSI string
        dsw = GDALDatasetWrapper(vsi_str)
        assert dsw is not None
        dsw = None

        # Cleanup
        ds_inmem = None
        ds = None

    def test_invalid_vfs(self):
        # Easy, give it a bad path
        with pytest.raises(RuntimeError):
            GDALDatasetWrapper('/vsizip//bad.zip/bad.gpkg')
