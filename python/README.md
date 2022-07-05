# Using the `exactextract` Python bindings

## Dependencies
The `exactextract` Python bindings require a standard Python 3.7+ installation with `pip` and `setuptools`. After building, the only extra dependency is GDAL.

## Building
With an Anaconda installation, run:
```bash
conda create -n my_env "python>=3.7" "gdal>=2.0.0" "pybind11>=2.2" cmake
conda activate my_env
python3 setup.py bdist
python3 setup.py install
```

Additionally, the following can be added to the build environment Docker file to enable building the Python bindings and docs.
```Dockerfile
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive && \
    apt-get install -y \
    python3-dev \
    python3-pip \
    python3-setuptools \
    python3-sphinx \
    pybind11-dev && \
    export CPLUS_INCLUDE_PATH=/usr/include/gdal && \
    export C_INCLUDE_PATH=/usr/include/gdal && \
    python3 -m pip install \
      pytest \
      yapf \
      pylama \
      "gdal==2.4.0" && \
    rm -rf /var/lib/apt/lists/*
```
A couple notes:
- `python3-sphinx` is only needed for building documentation.
- `pytest`, `yapf`, and `pylama` are only needed for linting if developing against the library.
- The GDAL version must be equal to the GDAL version on the system! Make sure to set the `C_INCLUDE_PATH` and `CPLUS_INCLUDE_PATH` so `pip` can find your GDAL header files.

To build the Python bindings, run:
```bash
python3 setup.py bdist
python3 setup.py install
```

To build the Python binding documentation, run:
```bash
mkdir -p build/pydocs
sphinx-apidoc -f -o ./pydocs ./python/src/exactextract && \
sphinx-build -b html ./pydocs ./build/pydocs
```

## Usage (or skip to the [TL;DR](#tldr))

### Import the module:
```python
from exactextract import GDALDatasetWrapper, GDALRasterWrapper, Operation, MapWriter, FeatureSequentialProcessor, GDALWriter
```

### Load a dataset:
Load the dataset you want to process. A few examples:
```python
# Load from file
dsw = GDALDatasetWrapper('/path/to/dataset.geojson', **kwargs)
dsw = GDALDatasetWrapper.from_descriptor('test.gpkg[layer_name]', **kwargs)  # like the CLI

# Load from existing dataset
from osgeo import ogr
ds = ogr.Open('/path/to/dataset.geojson')
dsw = GDALDatasetWrapper(ds, **kwargs)
```
---
**`GDALDatasetWrapper` Parameters**
    
* **filename_or_ds** (*Union**[**str**, **pathlib.Path**, **gdal.Dataset**, **ogr.DataSource**]*) – File path or OSGeo Dataset / DataSource


* **layer_name** (*Optional**[**str**]**, **optional*) – Optional name of layer.


* **layer_idx** (*Optional**[**int**]**, **optional*) – Optional layer index.


* **field_name** (*Optional**[**str**]**, **optional*) – Optional ID field name.


* **field_idx** (*Optional**[**int**]**, **optional*) – Optional ID field index.

**`GDALDataset.from_descriptor` Parameters**
    
* **desc** (*str*) – exactextract stat descriptor (example: `'test.gpkg[layer_name]'`)


* **field_name** (*Optional**[**str**]**, **optional*) – Optional ID field name. Defaults to None.


* **field_idx** (*Optional**[**int**]**, **optional*) – Optional ID field index. Defaults to None.

---

If you do not specify any of `layer_name` / `layer_idx` or `field_name` / `field_idx`, the first layer or field name will be chosen, respectively.

### Load a Raster
Load the raster you want to process. A few examples:
```python
# Load from file
rw = GDALRasterWrapper('/path/to/raster.tif', **kwargs)
rw = GDALRasterWrapper.from_descriptor('temp2:temperature.tif[4]', **kwargs)  # like the CLI

# Load from existing dataset
from osgeo import gdal
ds = gdal.Open('/path/to/raster.tif')
rw = GDALRasterWrapper(ds, **kwargs)
```
---
**`GDALRasterWrapper` Parameters**
    
* **filename_or_ds** (*Union**[**str**, **pathlib.Path**, **gdal.Dataset**]*) – File path or OSGeo Dataset / DataSource

* **band_idx** (*int**, **optional*) – Raster band index. Defaults to 1.

**`GDALRasterWrapper.from_descriptor` Parameters**

* **desc** (*str*) – exactextract stat descriptor (example: `'temp:NETCDF:outputs.nc:tmp2m'`, `'temp:temperature.tif[4]'`)
---

The first raster band is selected if one is not specified.

### Define your operations

Next is to definte your operations. A few examples:
```python
op = Operation('mean', 'this_id', raster=rw)
op = Operation('max', 'this_id', raster=rw, weights=rw2)

op = Operation.from_descriptor('mean(this_id)', raster=rw)
op2 = Operation.from_descriptor('max(this_id)', raster=rw, weights=rw2)
```
---
**`Operation` Parameters**
    
* **stat_name** (*str*) – Name of the stat. Refer to docs for options.


* **field_name** (*str*) – Field name to use. Output of operation will have title ‘{field_name}_{stat_name}’


* **raster** (*GDALRasterWrapper*) – Raster to compute over.


* **weights** (*Optional**[**GDALRasterWrapper**]**, **optional*) – Weight raster to use. Defaults to None.


**`Operation.from_descriptor` Parameters**

    
* **desc** (*str*) – exactextract stat descriptor (example: ‘mean(temp)’)


* **raster** (*GDALRasterWrapper*) – exactextract GDALRasterWrapper to compute the stat over


* **weights** (*Optional**[**GDALRasterWrapper**]**, **optional*) – Optional exactextract GDALRasterWrapper for weightrs. Defaults to None.

---

### Define your output writer
Define your output writer. There are two options: `MapWriter` to write to a Python `dict` object. Or `GDALWriter` to use to one of the support GDAL output drivers. Some examples:
```python
# Write to a Python dict
writer = MapWriter() 

# Use a GDAL driver
writer = GDALWriter('/path/to/output.dbf', dsw)

from osgeo import ogr
driver = ogr.GetDriverByName('CSV')
ds = driver.CreateDataSource('/path/to/output.csv')
writer = GDALWriter(ds, dsw)  # dsw is a reference GDALDatasetWrapper to get the ID field from
```
*Note:*
- `MapWriter`: use `writer.output` to get the output after processing.
- `GDALWriter`: set `writer = None` after processing to flush changes to the disk.
---
**`MapWriter` Parameters:**
  
* None

**`MapWriter` Methods:**

* #### `@property` output()
    Returns output map / dict from the writer


    * **Returns**

        Output map / dict



    * **Return type**

        Dict[str, List[float]]

**`GDALWriter` Parameters:**
  
* **filename_or_ds** (*Union**[**str**, **pathlib.Path**, **gdal.Dataset**, **ogr.DataSource**]*) – File path or OSGeo Dataset / DataSource

* **input_ds** (*GDALDatasetWrapper*) – Reference dataset to work with. This will be used to get the correct field to write.
---
### Run your Process:
```python
# Feature sequential Processor
processor = FeatureSequentialProcessor(dsw, writer, [op, op2])
processor.process()

# Raster Sequential Processor
processor = RasterSequentialProcessor(dsw, writer, [op, op2])
processor.process()

# If using MapWriter, we can fetch the output
writer.output

# If using GDALWriter, destroy to flush to disk
writer = None
```
---
**`FeatureSequentialProcessor` Parameters**
 
* **ds_wrapper** (*GDALDatasetWrapper*) – Dataset to use


* **writer** (*Union**[**MapWriter**, **GDALWriter**]*) – Writer to use


* **op_list** (*List**[**Operation**]*) – List of operations


**`RasterSequentialProcessor` Parameters**

    
* **ds_wrapper** (*GDALDatasetWrapper*) – Dataset to use


* **writer** (*Union**[**MapWriter**, **GDALWriter**]*) – Writer to use


* **op_list** (*List**[**Operation**]*) – List of operations
---
And that's it!

### TL;DR
```python
from exactextract import GDALDatasetWrapper, GDALRasterWrapper, Operation, MapWriter, FeatureSequentialProcessor, GDALWriter

# Define dataset wrapper
dsw = GDALDatasetWrapper('/path/to/dataset.gpkg')

# Define raster wrapper
rsw = GDALRasterWrapper('/path/to/raster.tif')

# Define operations
op = Operation.from_descriptor('mean(my_id)', raster=rsw)
op2 = Operation.from_descriptor('max(my_id)', raster=rsw)

# Define output writer
writer = GDALWriter('/path/to/my_output.csv', dsw)

# Process the data!
processor = FeatureSequentialProcessor(dsw, writer, [op, op2])
processor.process()

# Flush changes to disk
writer = None
```

## Running the unit tests
With `pytest` installed, simply run the following in the main project directory.
```bash
pytest  # run the tests
pytest -s  # run the tests, and see any captured output
```
