Command-line usage examples
===========================

Basic usage
-----------

A minimal usage is as follows, in which we want to compute a mean temperature for each country:

.. code-block:: bash

    exactextract \
      -r temperature_2018.tif \
      -p countries.shp \
      -s mean \
      -o mean_temperature.csv \
      --include-col country_name


In this example, ``exactextract`` will summarize temperatures stored in ``temperature_2018.tif`` over the country boundaries stored in ``countries.shp``.
    - The ``-r`` argument provides the location for of the raster input. The location may be specified as a filename or any other location understood by GDAL.
      For example, a single variable within a netCDF file can be accessed using ``-r temp:NETCDF:outputs.nc:tmp2m``.
      In files with more than one band, the band number (1-indexed) can be specified using square brackets, e.g., ``-r temperature.tif[4]``. If the band number
      is not specified, statistics will be output for all bands.
    - The ``-p`` argument provides the location for the polygon input.
      As with the ``-r`` argument, this can be a file name or some other location understood by GDAL, such as a PostGIS vector source (``-p    "PG:dbname=basins[public.basins_lev05]"``).
    - The ``-s`` argument instructs `exactextract` to compute the mean for each polygon.
      These values will be stored as a field called `mean` in the output file.
    - The ``-o`` argument indicates the location of the output file.
      The format of the output file is inferred using the file extension.
    - The ``--include-col`` argument indicates that we'd like the field ``country_name`` from the shapefile to be included as a field in the output file.

Multiple input files
--------------------

With reasonable real-world inputs, the processing time of ``exactextract`` is roughly divided evenly between (a) I/O (reading raster cells, which may require decompression) and (b) computing the area of each raster cell that is covered by each polygon.
In common usage, we might want to perform many calculations in which one or both of these steps can be reused, such as:

  * Computing the mean, min, and max temperatures in each country
  * Computing the mean temperature for several different years, each of which is stored in a separate but congruent raster files (having the same extent and resolution)

The following more advanced usage shows how ``exactextract`` might be called to perform multiple calculations at once, reusing work where possible:

.. code-block:: bash

    exactextract \
      -r "temperature_2016.tif" \
      -r "temperature_2017.tif" \
      -r "temperature_2018.tif" \
      -p countries.shp \
      -include-col country_name \
      -s "min" \
      -s "mean" \
      -s "max" \
      -o temp_summary.csv

In this case, the output ``temp_summary.csv`` file would contain the fields ``temperature_2016_min``, ``temperature_2016_mean``, etc. Each raster would be read only a single time, and each polygon/raster overlay would be performed a single time, because the three input rasters have the same extent and resolution.

Weighted summary operations
---------------------------

Another more advanced usage of ``exactextract`` involves calculations in which the values of one raster are weighted by the values of a second raster.
For example, we may wish to calculate both a standard and population-weighted mean temperature for each country:

.. code-block:: bash

    exactextract \
      -r "temperature_2018.tif" \
      -w "world_population.tif" \
      -p countries.shp \
      -f country_name \
      -s "mean" \
      -s "pop_weighted_mean=weighted_mean" \
      -o mean_temperature.csv


This also demonstrates the ability to control the name of a stat's output column by prefixing the stat name with an output column name.
