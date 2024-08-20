exactextract
============

``exactextract`` is a library for extracting and summarizing the values
in the portion of a raster dataset that is covered by a polygon, often referred
to as `zonal statistics`. 
The goals of ``exactextract`` are to be:

- fast - the :ref:`algorithm <algorithm>` used by ``exactextract`` outperforms many other implementations by avoiding point-in-polygon tests used to determine whether pixels are inside or outside the raster.
- accurate - ``exactextract`` considers partially-covered pixels by computing the fraction of each pixel that is covered by a polygon and weighting computations using this coverage fraction
- scalable - ``exactextract`` avoids the need to load polygon or raster datasets into memory at a single time, allowing it to be used on arbitrarily large datasets with fixed system resources.
- convenient - ``exactextract`` provides efficient implementations of many common :doc:`summary functions <operations>` and allows the user to define their own functions in R or Python.

The library is developed in C++, with bindings for Python and R.
It also includes a command-line interface.

Python
^^^^^^

.. currentmodule:: exactextract

Most tasks can be accomplished using the single function :py:func:`exact_extract`,
which provides a high-level interface:

.. code-block:: python

   from exactextract import exact_extract

   rast = "elevation.tif"
   polys = "watersheds.shp"

   exact_extract(rast, polys, ["mean", "min", "max"], include_cols=["watershed_id"])

This will return a list of GeoJSON-like features with properties containing the
values of the requested stats. Other output formats, such as pandas data frames
and GDAL datasets can be requested. :py:func:`exact_extract` is able to work with
inputs from many Python packages for spatial data, such as ``gdal``, ``rasterio``, 
``fiona``, ``geopandas``, and ``xarray``.
The documentation for :py:func:`exact_extract` describes the various types of
inputs and outputs that can be used.

For more information on the Python interface, see the :doc:`examples_python`
and :doc:`API documentation <autoapi/exactextract/index>`.

R
^

Usage in R is very similar to Python and primarily consists of calls to the ``exact_extract``
function.

.. code-block:: R

   library(exactextractr)
   library(terra)
   library(sf)

   elev <- rast("elevation.tif")
   polys <- st_read("watersheds.shp")
   exact_extract(rast, polys, c("mean", "min", "max"), append_cols='watershed_id')

The R package provides additional functionality, with adaptations of the exactextract
algorithm to rasterize polygonal data or resample gridded data.

More information is available in `R API documentation <https://isciences.gitlab.io/exactextractr/reference/index.html>`__ and
`usage examples <https://isciences.gitlab.io/exactextractr/articles/>`__. 

Command-line
^^^^^^^^^^^^

``exactextract`` provides a simple command-line interface that uses GDAL to read a vector data source and one or more raster files, perform zonal statistics, and write output to a CSV, netCDF, or other tabular formats supported by GDAL.

.. code-block:: bash

   exactextract \
  -r elevation.tif \
  -p watersheds.shp \
  -s "mean" \
  -s "min" \
  -s "max" \
  -o mean_temperature.csv \
  --include-col watershed_id

For more information, see :doc:`examples_cli`.

.. toctree::
   :maxdepth: 2
   :hidden:
   :caption: Contents:

   About <self>
   background
   operations
   installation
   faq
   CLI usage examples <examples_cli>
   Python usage examples <examples_python>
   Python API documentation <autoapi/exactextract/index>
   R usage examples <https://isciences.gitlab.io/exactextractr/articles/>
   R API documentation <https://isciences.gitlab.io/exactextractr/reference/index.html>
