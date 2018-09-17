# exactextract

[![Build Status](https://gitlab.com/isciences/exactextract/badges/master/build.svg)](https://gitlab.com/isciences/exactextract/pipelines)

`exactextract` provides a fast and accurate algorithm for summarizing values in the portion of a raster dataset that is covered by a polygon, often referred to as **zonal statistics**. Unlike other zonal statistics implementations, it takes into account raster calls that are partially covered by the polygon.

<img align="right" width="380" height="380" src="https://wsim.isciences.com/exactextract.svg" />

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

`exactextract` requires C++14 to build. It can be built as follows on Linux, using CMake 3.7 or greater:

```bash
git clone https://github.com/isciences/exactextract
cd exactextract
mkdir cmake-build-release
cd cmake-build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install
```

No testing has yet been performed on Windows or OS X.
Feedback concerning building on these platforms is welcome.

### Using `exactextract`

`exactextract` provides a simple command-line executable that uses GDAL to read a raster file and a shapefile and write various zonal statistics to a CSV.
Command line documentation can be accessed by `exactextract -h`.

An example usage is as follows:

```bash
exactextract \
  -r temp.tif \
  -w runoff.tif \
  -f NAME \
  -p ne_110m_admin_0_countries.shp \
  -s mean \
  -o runoff_weighted_mean_temperature.csv
```

In addition to the command-line executable, an R package ([`exactextractr`](https://github.com/isciences/exactextractr)) allows the functionality of `exactextract` to be used with `sf` and `raster` objects.

### Limitations

While `exactextract` does not need to load the entire raster into memory, it does need to load the portion of a raster that covers a given polygon. This limitation will be removed in the future.
