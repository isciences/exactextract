#include <limits>

#include "catch.hpp"

#include "grid.h"
#include "matrix.h"
#include "raster_area.h"

using namespace exactextract;

TEST_CASE("Cartesian area raster returns pixel area") {
    double dx = 1.0 / 3;
    double dy = 1.0 / 4;

    Grid<bounded_extent> g{{0, 0, 10, 10}, dx, dy};

    CartesianAreaRaster<double> areas(g);

    CHECK( areas(4, 3) == dx * dy );
}

TEST_CASE("Spherical area raster returns pixel area") {
    double dx = 1.0;
    double dy = 1.0;

    Grid<bounded_extent> g{{0, 45, 10, 55}, dx, dy};

    SphericalAreaRaster<double> areas(g);

    // Compare to result from PostGIS, which is more accurate because
    // it is performing a geodesic calculation using geographiclib.
    // SELECT ST_Area('POLYGON ((3 50, 4 50, 4 51, 3 51, 3 50))'::geography);
    constexpr double postgis_area = 7892061583.206543;

    CHECK( std::abs((areas(4, 3) - postgis_area) / postgis_area) < 0.002 );
}
