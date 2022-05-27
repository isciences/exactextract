# exactextract

[![Build Status](https://gitlab.com/isciences/exactextract/badges/master/pipeline.svg)](https://gitlab.com/isciences/exactextract/pipelines)
[![codecov](https://codecov.io/gl/isciences/exactextract/branch/master/graph/badge.svg)](https://codecov.io/gl/isciences/exactextract)
[![Doxygen](https://img.shields.io/badge/Doxygen-documentation-brightgreen.svg)](https://isciences.gitlab.io/exactextract)

`exactextract` provides a fast and accurate algorithm for summarizing values in the portion of a raster dataset that is covered by a polygon, often referred to as **zonal statistics**. Unlike other zonal statistics implementations, it takes into account raster cells that are partially covered by the polygon.

<img align="right" width="380" height="380" src="https://gitlab.com/isciences/exactextract/-/raw/master/doc/exactextract.svg" />

### Background

Accurate zonal statistics calculation requires determining the fraction of each raster cell that is covered by the polygon. In a naive solution to the problem, each raster cell can be expressed as a polygon whose intersection with the input polygon is computed using polygon clipping routines such as those offered in [JTS](https://github.com/locationtech/jts), [GEOS](https://github.com/OSGeo/geos), [CGAL](https://github.com/CGAL/cgal), or other libraries. However, polygon clipping algorithms are relatively expensive, and the performance of this approach is typically unacceptable unless raster resolution and polygon complexity are low. 

To achieve better performance, most zonal statistics implementations sacrifice accuracy by assuming that each cell of the raster is either wholly inside or outside of the polygon. This inside/outside determination can take various forms, for example:

- ArcGIS rasterizes the input polygon, then extracts the raster values from cells within the input polygon. Cells are interpreted to be either wholly within or outside of the polygon, depending on how the polygon is rasterized.
- [QGIS](https://qgis.org/en/site/) compares the centroid of each raster cell to the polygon boundary, initially considering cells to be wholly within or outside of the polygon based on the centroid. However, if fewer than two cell centroids fall within the polygon, an exact vector-based calculation is performed instead ([source](https://github.com/qgis/QGIS/blob/d5626d92360efffb4b8085389c8d64072ef65833/src/analysis/vector/qgszonalstatistics.cpp#L266)).
- Python's [rasterstats](https://pythonhosted.org/rasterstats/) also considers cells to be wholly within or outside of the polygon, but allows the user to decide to include cells only if their centroid is within the polygon, or if any portion of the cell touches the polygon ([docs](https://pythonhosted.org/rasterstats/manual.html#rasterization-strategy)).
- R's [raster](https://cran.r-project.org/web/packages/raster/index.html) package also uses a centroid test to determine if cells are inside or outside of the polygon. It includes a convenient method of disaggregating the raster by a factor of 10 before performing the analysis, which reduces the error incurred by ignoring partially covered cells but reduces performance substantially ([source](https://github.com/cran/raster/blob/4d218a7565d3994682557b8ae4d5b52bc2f54241/R/rasterizePolygons.R#L415)). The [velox](https://cran.r-project.org/web/packages/velox/index.html) package provides a faster implementation of the centroid test but does not provide a method for disaggregation. 

### Method used in `exactextract`

`exactextract` computes the portion of each cell that is covered by a polygon using an algorithm that proceeds as follows:

1. Each ring of a polygon is traversed a single time, making note of when it enters or exits a raster cell.
2. For each raster cell that was touched by a ring, the fraction of the cell covered by the polygon is computed. This is done by identifying all counter-clockwise-bounded areas within the cell.
3. Any cell that was not touched by the ring is known to be either entirely inside or outside of the polygon (i.e., its covered fraction is either `0` or `1`). A point-in-polygon test is used to determine which, and the `0` or `1` value is then propagated outward using a flood fill algorithm. Depending on the structure of the polygon, a handful of point-in-polygon tests may be necessary.

### Additional Features

`exactextract` can compute statistics against two rasters simultaneously, with a second raster containing weighting values.
The weighting raster does not need to have the same resolution and extent as the value raster, but the resolutions of the two rasters must be integer multiple of each other, and any difference between the grid origin points must be an integer multiple of the smallest cell size.

### Compiling

`exactextract` requires the following:

* A C++14 compiler (e.g., gcc 5.0+)
* CMake 3.8+
* [GEOS](https://github.com/libgeos/geos) version 3.5+
* [GDAL](https://github.com/osgeo/GDAL) version 2.0+ (For CLI binary)

It can be built as follows on Linux as follows:

```bash
git clone https://github.com/isciences/exactextract
cd exactextract
mkdir cmake-build-release
cd cmake-build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install
```

There are three options available to control what gets compiled. They are each ON by default.
- `BUILD_CLI` will build main program (which requires GDAL)
- `BUILD_TEST` will build the catch_test suite
- `BUILD_DOC` will build the doxygen documentation if doxygen is available

To build just the library and test suite, you can use these options as follows to turn off the CLI (which means GDAL isn't required) and disable the documentation build. The tests and library are built, the tests run, and the library installed if the tests were run successfully:

```bash
git clone https://github.com/isciences/exactextract
cd exactextract
mkdir cmake-build-release
cd cmake-build-release
cmake -DBUILD_CLI:=OFF -DBUILD_DOC:=OFF -DCMAKE_BUILD_TYPE=Release ..
make
./catch_tests && sudo make install
```

### Using `exactextract`

`exactextract` provides a simple command-line interface that uses GDAL to read a vector data source and one or more raster files, perform zonal statistics, and write output to a CSV, netCDF, or other tabular formats supported by GDAL.
In addition to the command-line executable, an R package ([`exactextractr`](https://github.com/isciences/exactextractr)) allows some functionality of `exactextract` to be used with R `sf` and `raster` objects.

Command line documentation can be accessed by `exactextract -h`.

A minimal usage is as follows, in which we want to compute a mean temperature for each country:

```bash
exactextract \
  -r temp:temperature_2018.tif \
  -p countries.shp \
  -f country_name \
  -s mean(temp) \
  -o mean_temperature.csv
```

In this example, `exactextract` will summarize temperatures stored in `temperature_2018.tif` over the country boundaries stored in `countries.shp`.
  * The `-r` argument provides the location for of the raster input and specifies that we'd like to refer to it later on using the name `temp`.
    The location may be specified as a filename or any other location understood by GDAL.
    For example, a single variable within a netCDF file can be accessed using `-r temp:NETCDF:outputs.nc:tmp2m`.
    In files with more than one band, the band number (1-indexed) can be specified using square brackets, e.g., `-r temp:temperature.tif[4]`.
  * The `-p` argument provides the location for the polygon input.
    As with the `-r` argument, this can be a file name or some other location understood by GDAL, such as a PostGIS vector source (`-p "PG:dbname=basins[public.basins_lev05]"`).
  * The `-f` argument indicates that we'd like the field `country_name` from the shapefile to be included as a field in the output file.
  * The `-s` argument instructs `exactextract` to compute the mean of the raster we refer to as `temp` for each polygon.
    These values will be stored as a field called `temp_mean` in the output file.
  * The `-o` argument indicates the location of the output file.
    The format of the output file is inferred by GDAL using the file extension.

With reasonable real-world inputs, the processing time of `exactextract` is roughly divided evenly between (a) I/O (reading raster cells, which may require decompression) and (b) computing the area of each raster cell that is covered by each polygon.
In common usage, we might want to perform many calculations in which one or both of these steps can be reused, such as:

  * Computing the mean, min, and max temperatures in each country
  * Computing the mean temperature for several different years, each of which is stored in a separate but congruent raster files (having the same extent and resolution)

The following more advanced usage shows how `exactextract` might be called to perform multiple calculations at once, reusing work where possible:

```bash
exactextract \
  -r temp_2016:temperature_2016.tif \
  -r temp_2017:temperature_2017.tif \
  -r temp_2018:temperature_2018.tif \
  -p countries.shp \
  -f country_name \
  -s min(temp_2016) \
  -s mean(temp_2016) \
  -s max(temp_2016) \
  -s min(temp_2017) \
  -s mean(temp_2017) \
  -s max(temp_2017) \
  -s min(temp_2017) \
  -s mean(temp_2017) \
  -s max(temp_2017) \
  -o temp_summary.csv
```

In this case, the output `temp_summary.csv` file would contain the fields `min_temp_2016`, `mean_temp_2016`, etc. Each raster would be read only a single time, and each polygon/raster overlay would be performed a single time, because the three input rasters have the same extent and resolution.

Another more advanced usage of `exactextract` involves calculations in which the values of one raster are weighted by the values of a second raster.
For example, we may wish to calculate both a standard and population-weighted mean temperature for each country:

```bash
exactextract \
  -r temp:temperature_2018.tif \
  -r pop:world_population.tif \
  -p countries.shp \
  -f country_name \
  -s mean(temp) \
  -s "pop_weighted_mean=weighted_mean(temp,pop)" \
  -o mean_temperature.csv
```

This also demonstrates the ability to control the name of a stat's output column by prefixing the stat name with an output column name.

Further details on weighted statistics are provided in the section below.

### Supported Statistics

The statistics supported by `exactextract` are summarized in the table below.
A formula is provided for each statistic, in which
x<sub>i</sub> represents the value of the *ith* raster cell,
c<sub>i</sub> represents the fraction of the *ith* raster cell that is covered by the polygon, and
w<sub>i</sub> represents the weight of the *ith* raster cell.
Values in the "example result" column refer to the value and weighting rasters shown below.
In these images, values of the "value raster" range from 1 to 4, and values of the "weighting raster" range from 5 to 8.
The area covered by the polygon is shaded purple.

| Example Value Raster | Example Weighting Raster |
| -------------------- | ------------------------ |
| <img align="left" width="200" height="200" src="https://gitlab.com/isciences/exactextract/-/raw/master/doc/readme_example_values.svg" /> | <img align="left" width="200" height="200" src="https://gitlab.com/isciences/exactextract/-/raw/master/doc/readme_example_weights.svg" /> | 


| Name           | Formula                                                                              | Description                                                                     | Typical Application  | Example Result |
| -------------- | ------------------------------------------------------------------------------------ | ------------------------------------------------------------------------------- | -------------------- |--------------- |
| count          | &Sigma;c<sub>i</sub>                                                                 | Sum of all cell coverage fractions. | | 0.5 + 0 + 1 + 0.25 = 1.75 |
| sum            | &Sigma;x<sub>i</sub>c<sub>i</sub>                                                    | Sum of values of raster cells that intersect the polygon, with each raster value weighted by its coverage fraction. | Total population | 0.5&times;1 + 0&times;2 + 1.0&times;3 + 0.25&times;4 = 4.5 |
| mean           | (&Sigma;x<sub>i</sub>c<sub>i</sub>)/(&Sigma;c<sub>i</sub>)                           | Mean value of cells that intersect the polygon, weighted by the percent of the cell that is covered. | Average temperature | 4.5/1.75 = 2.57 |
| weighted_sum   | &Sigma;x<sub>i</sub>c<sub>i</sub>w<sub>i</sub>                                       | Sum of raster cells covered by the polygon, with each raster value weighted by its coverage fraction and weighting raster value. | Total crop production lost | 0.5&times;1&times;5 + 0&times;2&times;6 + 1.0&times;3&times;7 + 0.25&times;4&times;8 = 31.5
| weighted_mean  | (&Sigma;x<sub>i</sub>c<sub>i</sub>w<sub>i</sub>)/(&Sigma;c<sub>i</sub>w<sub>i</sub>) | Mean value of cells that intersect the polygon, weighted by the product over the coverage fraction and the weighting raster. | Population-weighted average temperature | 31.5 / (0.5&times;5 + 0&times;6 + 1.0&times;7 + 0.25&times;8) = 2.74
| min            | -                                                                                    | Minimum value of cells that intersect the polygon, not taking coverage fractions or weighting raster values into account. | Minimum elevation | 1 |
| max            | -                                                                                    | Maximum value of cells that intersect the polygon, not taking coverage fractions or weighting raster values into account.  | Maximum temperature | 4 |
| minority       | -                                                                                    | The raster value occupying the least number of cells, taking into account cell coverage fractions but not weighting raster values. | Least common land cover type | - |
| majority       | -                                                                                    | The raster value occupying the greatest number of cells, taking into account cell coverage fractions but not weighting raster values. | Most common land cover type | - |
| variety        | -                                                                                    | The number of distinct raster values in cells wholly or partially covered by the polygon. | Number of land cover types | - |
| variance       | (&Sigma;c<sub>i</sub>(x<sub>i</sub> - x&#773;)<sup>2</sup>)/(&Sigma;c<sub>i</sub>)                | Population variance of cell values that intersect the polygon, taking into account coverage fraction. | - | 1.10 |
| stdev          | &Sqrt;variance                                                                       | Population standard deviation of cell values that intersect the polygon, taking into account coverage fraction. | - | 1.05 |
| coefficient_of_variation | stdev / mean                                                               | Population coefficient of variation of cell values that intersect the polygon, taking into account coverage fraction. | - | 0.41 |
