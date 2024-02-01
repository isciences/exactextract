exactextract
============

``exactextract`` is a Python library for extracting and summarizing the values
in the portion of a raster dataset that is covered by a polygon, often referred
to as `zonal statistics`. It is able to work with inputs from many Python
packages for spatial data, such as ``gdal``, ``rasterio``, ``fiona``,
``geopandas``, and ``xarray``.

.. currentmodule:: exactextract

Most tasks can be accomplished using the single function :py:func:`exact_extract`,
which provides a high-level interface similar to that of the ``exactextractr`` R package:

.. code-block:: python

   from exactextract import exact_extract

   rast = "elevation.tif"
   polys = "watersheds.shp"

   exact_extract(rast, polys, ["mean", "min", "max"], include_cols=["watershed_id"])

This will return a list of GeoJSON-like features with properties containing the
values of the requested stats. Other output formats, such as pandas data frames
and GDAL datasets can be requested. The documentation for
:py:func:`exact_extract` describes the various types of inputs and outputs that
can be used.


.. toctree::
   :maxdepth: 2
   :caption: Contents:



Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
