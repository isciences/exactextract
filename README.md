# exactextract

[![Build Status](https://github.com/isciences/exactextract/actions/workflows/linux_build.yml/badge.svg)
[![codecov](https://codecov.io/gh/isciences/exactextract/branch/master/graph/badge.svg)](https://codecov.io/gh/isciences/exactextract)

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
The weighting raster does not need to have the same resolution and extent as the value raster, but the resolutions of the two rasters must be integer multiples of each other, and any difference between the grid origin points must be an integer multiple of the smallest cell size.

### Compiling

`exactextract` requires the following:

* A C++17 compiler (e.g., gcc 7+)
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
- `BUILD_BENCHMARKS` will build performance tests
- `BUILD_CLI` will build a command-line interface (requires GDAL)
- `BUILD_DOC` will build the doxygen documentation if doxygen is available
- `BUILD_PYTHON` will build Python bindings (requires pybind11)
- `BUILD_TEST` will build the catch_test suite

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

In addition to the command-line executable, an R package ([`exactextractr`](https://github.com/isciences/exactextractr)) and [python bindings](python/README.md) allow the functionality of `exactextract` to be used with R `sf` and `raster` objects.

Command line documentation can be accessed by `exactextract -h`.

A minimal usage is as follows, in which we want to compute a mean temperature for each country:

```bash
exactextract \
  -r "temperature_2018.tif" \
  -p countries.shp \
  -s "mean" \
  -o mean_temperature.csv \
  --include-col country_name
```

In this example, `exactextract` will summarize temperatures stored in `temperature_2018.tif` over the country boundaries stored in `countries.shp`.
  * The `-r` argument provides the location for of the raster input. The location may be specified as a filename or any other location understood by GDAL.
    For example, a single variable within a netCDF file can be accessed using `-r temp:NETCDF:outputs.nc:tmp2m`.
    In files with more than one band, the band number (1-indexed) can be specified using square brackets, e.g., `-r temperature.tif[4]`. If the band number
    is not specified, statistics will be output for all bands.
  * The `-p` argument provides the location for the polygon input.
    As with the `-r` argument, this can be a file name or some other location understood by GDAL, such as a PostGIS vector source (`-p "PG:dbname=basins[public.basins_lev05]"`).
  * The `-s` argument instructs `exactextract` to compute the mean for each polygon.
    These values will be stored as a field called `mean` in the output file.
  * The `-o` argument indicates the location of the output file.
    The format of the output file is inferred using the file extension.
  * The `--include-col` argument indicates that we'd like the field `country_name` from the shapefile to be included as a field in the output file.

With reasonable real-world inputs, the processing time of `exactextract` is roughly divided evenly between (a) I/O (reading raster cells, which may require decompression) and (b) computing the area of each raster cell that is covered by each polygon.
In common usage, we might want to perform many calculations in which one or both of these steps can be reused, such as:

  * Computing the mean, min, and max temperatures in each country
  * Computing the mean temperature for several different years, each of which is stored in a separate but congruent raster files (having the same extent and resolution)

The following more advanced usage shows how `exactextract` might be called to perform multiple calculations at once, reusing work where possible:

```bash
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
```

In this case, the output `temp_summary.csv` file would contain the fields `temperature_2016_min`, `temperature_2016_mean`, etc. Each raster would be read only a single time, and each polygon/raster overlay would be performed a single time, because the three input rasters have the same extent and resolution.

Another more advanced usage of `exactextract` involves calculations in which the values of one raster are weighted by the values of a second raster.
For example, we may wish to calculate both a standard and population-weighted mean temperature for each country:

```bash
exactextract \
  -r "temperature_2018.tif" \
  -w "world_population.tif" \
  -p countries.shp \
  -f country_name \
  -s "mean" \
  -s "pop_weighted_mean=weighted_mean" \
  -o mean_temperature.csv
```

This also demonstrates the ability to control the name of a stat's output column by prefixing the stat name with an output column name.

Further details on weighted statistics are provided in the section below.

### Alternate syntax

`exactextract` 0.1 required a somewhat more verbose syntax in which each raster was required to have a name that could be provided as an argument to a stat function, e.g.:

```bash
exactextract \
  -r "temp:temperature_2018.tif" \
  -r "pop:world_population.tif" \
  -p countries.shp \
  -f country_name \
  -s "mean(temp)" \
  -s "pop_weighted_mean=weighted_mean(temp,pop)" \
  -o mean_temperature.csv
```

This syntax is still supported.

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
| cell_id        | - | Array with 0-based index of each cell that intersects the polygon, increasing left-to-right. | - | [ 0, 2, 3 ] 
| center_x       | -                                                                                    | Array with cell center x-coordinate for each cell that intersects the polygon. Each cell center may or may not be inside the polygon. | - | [ 0.5, 0.5, 1.5 ] |
| center_y       | -                                                                                    | Array with cell center y-coordinate for each cell that intersects the polygon. Each cell center may or may not be inside the polygon. | - | [ 1.5, 0.5, 0.5 ] |
| coefficient_of_variation | stdev / mean                                                               | Population coefficient of variation of cell values that intersect the polygon, taking into account coverage fraction. | - | 0.41 |
| count          | &Sigma;c<sub>i</sub>                                                                 | Sum of all cell coverage fractions. | | 0.5 + 0 + 1 + 0.25 = 1.75 |
| coverage       | - | Array with coverage fraction of each cell that intersects the polygon | - | [ 0.5, 1.0, 0.25 ]
| frac           | -                                                                                    | Fraction of covered cells that are occupied by each distinct raster value. | Land cover summary | - | 
| majority       | -                                                                                    | The raster value occupying the greatest number of cells, taking into account cell coverage fractions but not weighting raster values. | Most common land cover type | 3 |
| max            | -                                                                                    | Maximum value of cells that intersect the polygon, not taking coverage fractions or weighting raster values into account.  | Maximum temperature | 4 |
| max_center_x   | - | Cell center x-coordinate for the cell containing the maximum value intersected by the polygon. The center of this cell may or may not be inside the polygon. | Highest point in watershed | 1.5 |
| max_center_y   | - | Cell center y-coordinate for the cell containing the maximum value intersected by the polygon. The center of this cell may or may not be inside the polygon. | Highest point in watershed | 0.5 |
| mean           | (&Sigma;x<sub>i</sub>c<sub>i</sub>)/(&Sigma;c<sub>i</sub>)                           | Mean value of cells that intersect the polygon, weighted by the percent of each cell that is covered. | Average temperature | 4.5/1.75 = 2.57 |
| median         |                                                                                      | Median value of cells that intersect the polygon, weighted by the percent of each cell that is covered | Average temperature | 2 |
| min            | -                                                                                    | Minimum value of cells that intersect the polygon, not taking coverage fractions or weighting raster values into account. | Minimum elevation | 1 |
| min_center_x   | - | Cell center x-coordinate for the cell containing the minimum value intersected by the polygon. The center of this cell may or may not be inside the polygon. | Lowest point in watershed | 0.5 |
| min_center_y   | - | Cell center y-coordinate for the cell containing the minimum value intersected by the polygon. The center of this cell may or may not be inside the polygon. | Lowest point in watershed | 1.5 |
| minority       | -                                                                                    | The raster value occupying the least number of cells, taking into account cell coverage fractions but not weighting raster values. | Least common land cover type | 4 |
| quantile       | - | Specified quantile of cells that intersect the polygon, weighted by the percent of each cell that is covered. Quantile value is specified by the `q` argument, 0 to 1. | - |
| stdev          | &Sqrt;variance                                                                       | Population standard deviation of cell values that intersect the polygon, taking into account coverage fraction. | - | 1.05 |
| sum            | &Sigma;x<sub>i</sub>c<sub>i</sub>                                                    | Sum of values of raster cells that intersect the polygon, with each raster value weighted by its coverage fraction. | Total population | 0.5&times;1 + 0&times;2 + 1.0&times;3 + 0.25&times;4 = 4.5 |
| unique         |                                                                                      | Array of unique raster values for cells that intersect the polygon | - | [ 1, 3, 4 ] |
| values         |                                                                                      | Array of raster values for each cell that intersects the polygon | - | [ 1, 3, 4 ] |
| variance       | (&Sigma;c<sub>i</sub>(x<sub>i</sub> - x&#773;)<sup>2</sup>)/(&Sigma;c<sub>i</sub>)   | Population variance of cell values that intersect the polygon, taking into account coverage fraction. | - | 1.10 |
| variety        | -                                                                                    | The number of distinct raster values in cells wholly or partially covered by the polygon. | Number of land cover types | 3 |
| weighted_frac  | -                                                                                    | Fraction of covered cells that are occupied by each distinct raster value, weighted by the value of a second weighting raster. | Population-weighted land cover summary | - | 
| weighted_mean  | (&Sigma;x<sub>i</sub>c<sub>i</sub>w<sub>i</sub>)/(&Sigma;c<sub>i</sub>w<sub>i</sub>) | Mean value of cells that intersect the polygon, weighted by the product over the coverage fraction and the weighting raster. | Population-weighted average temperature | 31.5 / (0.5&times;5 + 0&times;6 + 1.0&times;7 + 0.25&times;8) = 2.74
| weighted_stdev |                                                                                      | Weighted version of `stdev`. | - |
| weighted_sum   | &Sigma;x<sub>i</sub>c<sub>i</sub>w<sub>i</sub>                                       | Sum of raster cells covered by the polygon, with each raster value weighted by its coverage fraction and weighting raster value. | Total crop production lost | 0.5&times;1&times;5 + 0&times;2&times;6 + 1.0&times;3&times;7 + 0.25&times;4&times;8 = 31.5
| weighted_variance |                                                                                   | Weighted version of `variance` | - |
| weights        |                                                                                      | Array of weight values for each cell that intersects the polygon | - | [ 5, 7, 8 ] |

The behavior of these statistics may be modified by the following arguments:

- `coverage_weight` - specifies the value to use for ci in the above formulas. The following methods are available:
   * `fraction` - the default method, with ci ranging from 0 to 1
   * `none` - ci is always equal to 1; all pixels are given the same weight in the above calculations regardless of their coverage fraction
   * `area_cartesian` - ci is the fraction of the pixel multiplied by it x and y resolutions
   * `area_spherical_m2` - ci is the fraction of the pixel that is covered multiplied by a spherical approximation of the cell's area in square meters
   * `area_spherical_km2` - ci is the fraction of the pixel that is covered multiplied by a spherical approximation of the cell's area in square kilometers
- `default_value` - specifies a value to be used in place of NODATA
- `default_weight` - specifies a weighing value to be used in place of NODATA
- `min_coverage_frac` - specifies the minimum fraction of the pixel (0 to 1) that must be covered in order for a pixel to be considered in calculations. Defaults to 0.
