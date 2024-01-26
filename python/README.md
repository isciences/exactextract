## Dependencies

The following dependencies are required to build the Python bindings:

- CMake
- [pybind11](https://github.com/pybind/pybind11)
- [GEOS](https://github.com/libgeos)

Unlike the command-line interface, GDAL is not a requirement, although the
Python bindings can work with the GDAL Python bindings if they are available.

## Installation

To install the Python bindings, run `pip install .` from the root of the source tree.

## Basic usage

In most cases, the simplest usage is the `exact_extract` function:

```python
from exactextract import exact_extract

rast = "elevation.tif"
polys = "watersheds.shp"

exact_extract(rast, polys, ["mean", "min", "max"])
```

This will return a list of GeoJSON-like features with properties containing the
values of the requested stats. `exact_extract` can understand several different
types of inputs:

- raster inputs: 
   - a GDAL `Dataset`
   - a rasterio `DatasetReader`
   - an xarray `DataArray`
   - a path to a file that can be opened with one of the above packages
   - a `RasterSource` object (such as a `NumPyRasterSource` that wraps
     a `numpy` array)

- vector inputs:
    - a GDAL/OGR `Dataset` or `DataSource`
    - a fiona `Collection`
    - a geopandas `GeoDataFrame`
    - a path to a file that can be opened with one of the above packages
    - a list of GeoJSON-like features
    - a `FeatureSource` object

The following additional arguments are understood by `exact_extract`:

- `weights`: a raster representing weights to use for stats such as
  `weighted_mean`
- `include_cols`: a list of columns from the input features to be copied into
  the output features
- `include_geom`: a flag indicating that the input geometry should be copied
  into the output features
- `strategy`: a processing strategy to use, either `feature-sequential`
  (iterate over features and read the pixels that intersect each feature) or
`raster-sequential` (iterate over chunks of the raster and process the features
that intersect each chunk)
- `max_cells_in_memory`: used to control the number of cells that are loaded at
  one time, for large features
- `output`: used to control the output format: either `geojson`, `pandas`, or
  the name of an output file to write to using GDAL.
