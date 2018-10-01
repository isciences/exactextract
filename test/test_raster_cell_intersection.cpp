#include <geos_c.h>

#include "catch.hpp"

#include "geos_utils.h"
#include "raster_cell_intersection.h"

using namespace exactextract;

void check_cell_intersections(Raster<float> & rci, const std::vector<std::vector<float>> & v) {
    const Matrix<float>& actual = rci.data();
    Matrix<float> expected{v};

    REQUIRE( expected.rows() == actual.rows() );
    REQUIRE( expected.cols() == actual.cols() );

    for (size_t i = 0; i < expected.rows(); i++) {
        for (size_t j = 0; j < expected.cols(); j++) {
            CHECK ( actual(i, j) == expected(i, j) );
        }
    }
}

static void init_geos() {
    static bool initialized = false;

    if (!initialized) {
        initGEOS(nullptr, nullptr);
        initialized = true;
    }
}

TEST_CASE("Basic", "[raster-cell-intersection]" ) {
    init_geos();

    Grid<bounded_extent> ex{{0, 0, 3, 3}, 1, 1}; // 3x3 grid

    auto g = GEOSGeom_read("POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2.5, 0.5 2.5, 0.5 0.5))");

    Raster<float> rci = raster_cell_intersection(ex, g.get());

    check_cell_intersections(rci, {
            {0.25, 0.5, 0.25},
            {0.50, 1.0, 0.50},
            {0.25, 0.5, 0.25}
    });
}

TEST_CASE("Geometry extent larger than raster", "[raster-cell-intersection]") {
    init_geos();

    // Process a geometry using four 3x3 tiles

    // +-----+-----+
    // |  1  |  2  |
    // +-----+-----+
    // |  3  |  4  |
    // +-----+-----+


    Box b3 {0, 0, 3, 3};
    Box b2 = b3.translate(3, 3);
    Box b1 = b3.translate(0, 3);
    Box b4 = b3.translate(3, 0);

    Grid<bounded_extent> g1 {b1, 1, 1};
    Grid<bounded_extent> g2 {b2, 1, 1};
    Grid<bounded_extent> g3 {b3, 1, 1};
    Grid<bounded_extent> g4 {b4, 1, 1};

    auto g = GEOSGeom_read("POLYGON ((0.5 0.5, 4.5 0.5, 4.5 5.5, 0.5 5.5, 0.5 0.5))");

    Raster<float> ll = raster_cell_intersection(g3, g.get());

    check_cell_intersections(ll, {
            {0.50, 1.0, 1.0},
            {0.50, 1.0, 1.0},
            {0.25, 0.5, 0.5}
    });

    Raster<float> lr = raster_cell_intersection(g4, g.get());

    check_cell_intersections(lr, {
            {1.00, 0.50},
            {1.00, 0.50},
            {0.50, 0.25}
    });

    Raster<float> ur = raster_cell_intersection(g2, g.get());

    check_cell_intersections(ur, {
            {0.50, 0.25},
            {1.00, 0.50},
            {1.00, 0.50}
    });

    Raster<float> ul = raster_cell_intersection(g1, g.get());

    check_cell_intersections(ul, {
            {0.25, 0.5, 0.5},
            {0.50, 1.0, 1.0},
            {0.50, 1.0, 1.0}
    });
}

TEST_CASE("Geometry entirely outside raster", "[raster-cell-intersection]") {
    init_geos();

    Grid<bounded_extent> ex{{-3, -3, 0, 0}, 1, 1}; // 3x3 grid

    auto g = GEOSGeom_read("POLYGON ((1.5 0.5, 2.5 1.5, 1.5 2.5, 0.5 1.5, 1.5 0.5))");

    Raster<float> rci = raster_cell_intersection(ex, g.get());

    CHECK ( rci.rows() == 0 );
    CHECK ( rci.cols() == 0 );
}

TEST_CASE("Invalid geometry with detached inner ring outside raster", "[raster-cell-intersection]") {
    init_geos();

    Grid<bounded_extent> ex{{0, 0, 3, 3}, 1, 1}; // 3x3 grid

    auto g = GEOSGeom_read("POLYGON ((1.5 0.5, 2.5 1.5, 1.5 2.5, 0.5 1.5, 1.5 0.5), (100 100, 100 101, 101 101, 100 100))");

    Raster<float> rci = raster_cell_intersection(ex, g.get());

    CHECK ( rci.rows() == 3 );
    CHECK ( rci.cols() == 3 );
}

TEST_CASE("Diagonals", "[raster-cell-intersection]") {
    init_geos();

    Grid<bounded_extent> ex{{0, 0, 3, 3}, 1, 1}; // 3x3 grid

    auto g = GEOSGeom_read("POLYGON ((1.5 0.5, 2.5 1.5, 1.5 2.5, 0.5 1.5, 1.5 0.5))");

    Raster<float> rci = raster_cell_intersection(ex, g.get());

    check_cell_intersections(rci, {
            {0.00, 0.25, 0.00},
            {0.25, 1.00, 0.25},
            {0.00, 0.25, 0.00},
    });
}

TEST_CASE("Starting on cell boundary", "[raster-cell-intersection]") {
    init_geos();

    // Situation found in Canada using 0.5-degree global grid
    Grid<bounded_extent> ex{{0, 0, 2, 2}, 1, 1}; // 2x2 grid

    auto g = GEOSGeom_read("POLYGON ((1 1.5, 1.5 1.5, 1.5 0.5, 0.5 0.5, 0.5 1.5, 1 1.5))");

    Raster<float> rci = raster_cell_intersection(ex, g.get());

    check_cell_intersections(rci, {
            {0.25, 0.25},
            {0.25, 0.25},
    });
}

TEST_CASE("Bouncing off boundary", "[raster-cell-intersection]") {
    init_geos();

    // Situation found in Trinidad and Tobago using 0.5-degree global grid
    Grid<bounded_extent> ex{{0, -1, 2, 2}, 1, 1}; // 3x2 grid

    auto g = GEOSGeom_read("POLYGON ((0.5 1.5, 0.5 0.5, 0.5 0, 1.5 0.5, 1.5 1.5, 0.5 1.5))");

    Raster<float> rci = raster_cell_intersection(ex, g.get());

    check_cell_intersections(rci, {
            {0.25,   0.25},
            {0.4375, 0.3125},
            {0.0,    0.0},
    });
}

TEST_CASE("Bouncing off boundary (2)", "[raster-cell-intersection]") {
    init_geos();

    Grid<bounded_extent> ex{{0, 0, 2, 2}, 1, 1};

    auto g = GEOSGeom_read("POLYGON ((0.5 0.5, 1.5 0.5, 1.5 1.5, 0.5 1.5, 1 1.2, 0.5 0.5))");

    CHECK_NOTHROW( RasterCellIntersection(ex, g.get()) );
}


TEST_CASE("Follows grid boundary", "[raster-cell-intersection]") {
    init_geos();

    // Occurs on the Libya-Egypt border, for example

    Grid<bounded_extent> ex{{0, 0, 3, 3}, 1, 1};

    auto g = GEOSGeom_read("POLYGON ((0.5 0.5, 2 0.5, 2 1.5, 2 2.5, 0.5 2.5, 0.5 0.5))");

    Raster<float> rci = raster_cell_intersection(ex, g.get());

    check_cell_intersections(rci, {
            {0.25, 0.5, 0.0},
            {0.50, 1.0, 0.0},
            {0.25, 0.5, 0.0},
    });
}

TEST_CASE("Starts on vertical boundary, moving up", "[raster-cell-intersection]") {
    init_geos();

    Grid<bounded_extent> ex{{0, 0, 4, 3}, 1, 1}; // 4x3 grid

    auto g = GEOSGeom_read("POLYGON ((3 0.5, 3 2.5, 0.5 2.5, 0.5 0.5, 3 0.5))");

    Raster<float> rci = raster_cell_intersection(ex, g.get());

    check_cell_intersections(rci, {
            {0.25, 0.5, 0.5, 0.0},
            {0.50, 1.0, 1.0, 0.0},
            {0.25, 0.5, 0.5, 0.0},
    });
}

TEST_CASE("Starts on vertical boundary, moving down", "[raster-cell-intersection]") {
    init_geos();

    Grid<bounded_extent> ex{{0, 0, 4, 3}, 1, 1}; // 4x3 grid

    auto g = GEOSGeom_read("POLYGON ((0.5 2.5, 0.5 0.5, 3 0.5, 3 2.5, 0.5 2.5))");

    Raster<float> rci = raster_cell_intersection(ex, g.get());

    check_cell_intersections(rci, {
            {0.25, 0.5, 0.5, 0.0},
            {0.50, 1.0, 1.0, 0.0},
            {0.25, 0.5, 0.5, 0.0},
    });
}

TEST_CASE("Starts on vertical boundary, moving down at rightmost extent of grid", "[raster-cell-intersection]") {
    init_geos();

    Grid<bounded_extent> ex{{0, 0, 3, 3}, 1, 1}; // 3x3 grid

    auto g = GEOSGeom_read("POLYGON ((3 2.5, 3 0.5, 0.5 0.5, 0.5 2.5, 3 2.5))");

    Raster<float> rci = raster_cell_intersection(ex, g.get());

    check_cell_intersections(rci, {
            {0.25, 0.5, 0.5},
            {0.50, 1.0, 1.0},
            {0.25, 0.5, 0.5},
    });
}

TEST_CASE("Starts on horizontal boundary, moving right", "[raster-cell-intersection]") {
    init_geos();

    Grid<bounded_extent> ex{{0, 0, 3, 4}, 1, 1}; // 3x4 grid

    auto g = GEOSGeom_read("POLYGON ((0.5 1, 2.5 1, 2.5 3.5, 0.5 3.5, 0.5 1))");

    Raster<float> rci = raster_cell_intersection(ex, g.get());

    check_cell_intersections(rci, {
            {0.25, 0.5, 0.25},
            {0.50, 1.0, 0.50},
            {0.50, 1.0, 0.50},
            {0.00, 0.0, 0.00},
     });
}

TEST_CASE("Starts on horizontal boundary, moving left", "[raster-cell-intersection]") {
    init_geos();

    Grid<bounded_extent> ex{{0, 0, 3, 4}, 1, 1}; // 3x4 grid

    auto g = GEOSGeom_read("POLYGON ((2.5 3, 0.5 3, 0.5 3.5, 0.25 3.5, 0.25 0.5, 2.5 0.5, 2.5 3))");

    Raster<float> rci = raster_cell_intersection(ex, g.get());

    check_cell_intersections(rci, {
            {0.125, 0.00, 0.00},
            {0.750, 1.00, 0.50},
            {0.750, 1.00, 0.50},
            {0.375, 0.50, 0.25},
    });
}

TEST_CASE("Regression test - Fiji", "[raster-cell-intersection]") {
    init_geos();

    // Just make sure this polygon doesn't throw an exception. It caused some problems where the
    // rightmost edge was interpreted to be exactly on a cell wall.
    Grid<bounded_extent> ex{{-180.5, -90.5, 180.5, 90.5}, 0.5, 0.5};

    auto g = GEOSGeom_read("MULTIPOLYGON (((178.3736000000001 -17.33992000000002, 178.71806000000007 -17.62845999999996, 178.5527099999999 -18.150590000000008, 177.93266000000008 -18.287990000000036, 177.38145999999992 -18.164319999999975, 177.28504000000007 -17.72464999999997, 177.67087 -17.381139999999974, 178.12557000000007 -17.50480999999995, 178.3736000000001 -17.33992000000002)), ((179.36414266196417 -16.801354076946836, 178.7250593629972 -17.012041674368007, 178.5968385951172 -16.63915000000003, 179.0966093629972 -16.43398427754741, 179.4135093629972 -16.379054277547382, 180.00000000000003 -16.06713266364241, 180.00000000000003 -16.555216566639146, 179.36414266196417 -16.801354076946836)), ((-179.91736938476527 -16.501783135649347, -179.99999999999997 -16.555216566639146, -179.99999999999997 -16.06713266364241, -179.79332010904858 -16.020882256741217, -179.91736938476527 -16.501783135649347)))");

    CHECK_NOTHROW( RasterCellIntersection(ex, g.get()) );
}

TEST_CASE("Small polygon", "[raster-cell-intersection]") {
    init_geos();

    Grid<bounded_extent> ex{{0, 0, 10, 10}, 10, 10}; // Single cell

    auto g = GEOSGeom_read("POLYGON ((3 3, 4 3, 4 4, 3 4, 3 3))");

    Raster<float> rci = raster_cell_intersection(ex, g.get());

    check_cell_intersections(rci, {{0.01}});
}

TEST_CASE("Fill handled correctly", "[raster-cell-intersection]") {
    init_geos();

    Grid<bounded_extent> ex{{0, 0, 3, 5}, 1, 1}; // 3x5 grid

    auto g = GEOSGeom_read("POLYGON ((0.5 0.2, 2.2 0.2, 2.2 0.4, 0.7 0.4, 0.7 2.2, 2.2 2.2, 2.2 0.6, 2.4 0.6, 2.4 4.8, 0.5 4.8, 0.5 0.2))");

    Raster<float> rci = raster_cell_intersection(ex, g.get());

    check_cell_intersections(rci, {
            {0.40, 0.80, 0.32},
            {0.50, 1.00, 0.40},
            {0.44, 0.80, 0.36},
            {0.20, 0.00, 0.20},
            {0.22, 0.20, 0.12},
    });
}

TEST_CASE("Result indexing is correct", "[raster-cell-intersection]") {
    init_geos();

    Grid<bounded_extent> ex{{-20, -15, 40, 30}, 0.5, 1};

    auto g = GEOSGeom_read("POLYGON ((0.25 0.20, 2.75 0.20, 2.75 4.5, 0.25 4.5, 0.25 0.20))");

    Raster<float> rci = raster_cell_intersection(ex, g.get());

    size_t n_rows = rci.rows();
    size_t n_cols = rci.cols();

    CHECK( n_rows == 5 );
    CHECK( n_cols == 6 );

    CHECK( rci.grid().col_offset(ex) == 40 );
    CHECK( rci.grid().row_offset(ex) == 25 );

    Matrix<float> expected{{
           {0.25, 0.50, 0.50, 0.50, 0.50, 0.25},
           {0.50, 1.00, 1.00, 1.00, 1.00, 0.50},
           {0.50, 1.00, 1.00, 1.00, 1.00, 0.50},
           {0.50, 1.00, 1.00, 1.00, 1.00, 0.50},
           {0.40, 0.80, 0.80, 0.80, 0.80, 0.40}
    }};

    CHECK( rci.data() == expected );
}

TEST_CASE("Robustness regression test #1", "[raster-cell-intersection]") {
    // This test exercises some challenging behavior where a polygon follows
    // ymin, but the grid resolution is such that ymin < (ymax - ny*dy)
    init_geos();

    Grid<bounded_extent> ex{{-180, -90, 180, 90}, 1.0/6, 1.0/6};

    auto g = GEOSGeom_read(
#include "resources/antarctica.wkt"
            );

    CHECK_NOTHROW( RasterCellIntersection(ex, g.get()) );
}

TEST_CASE("Robustness regression test #2", "[raster-cell-intersection]") {
    // This test exercises some challenging behavior where a polygon follows
    // xmax, but the grid resolution is such that xmax < (xmin + nx*m_dx)
    init_geos();

    Grid<bounded_extent> ex{{-180, -90, 180, 90}, 1.0/6, 1.0/6};

    auto g = GEOSGeom_read(
#include "resources/russia.wkt"
    );

    CHECK_NOTHROW( RasterCellIntersection(ex, g.get()) );
}

TEST_CASE("Robustness regression test #3", "[raster-cell-intersection]") {
    // The situation in this case was causing some kind of infinite loop, ultimately exhausting memory
    init_geos();

    Grid<bounded_extent> ex{{179.96666666664618, -16.541666666669137, 179.99999999997954,-16.475000000002474}, 0.0083333333333328596, 0.0083333333333328596};

    auto g = GEOSGeom_read("POLYGON ((179.9715827094184135 -16.5409617106119526,  180.0000000000000000 -16.5326999999999984, 179.9872884114583655 -16.5342697143554425,  179.9715827094184135 -16.5409617106119526))");

    CHECK_NOTHROW( raster_cell_intersection(ex, g.get()) );
}

TEST_CASE("Robustness regression test #4", "[raster-cell-intersection]") {
    init_geos();

    Grid<bounded_extent> ex{{-166.84166666666667, 66.991666666666674, -152.625, 71.358333333333334}, 0.0083333333333333332, 0.0083333333333333332};

    auto g = GEOSGeom_read(
#include "resources/regression4.wkt"
    );

    CHECK_NOTHROW( raster_cell_intersection(ex, g.get()) );
}