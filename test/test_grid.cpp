#include "catch.hpp"

#include "grid.h"

#include <numeric>

using namespace exactextract;

const Box global{-180, -90, 180, 90};

TEST_CASE("Infinite grid dimensions calculated correctly", "[grid]") {
    Grid<infinite_extent> grid{global, 0.5, 0.5};

    CHECK( grid.rows() == 2+360);
    CHECK( grid.cols() == 2+720);
}

TEST_CASE("Infinite grid dimension robustness", "[grid]") {
    Grid<infinite_extent> grid{{8.5, 1.6, 16.2, 13.1}, 0.1, 0.1};

    CHECK(grid.cols() == 2+77);
    CHECK(grid.rows() == 2+115);
}

TEST_CASE("Bounded grid dimensions calculated correctly", "[grid]") {
    Grid<bounded_extent> grid{global, 0.5, 0.5};

    CHECK( grid.rows() == 360);
    CHECK( grid.cols() == 720);
}

TEST_CASE("Bounded grid dimension robustness", "[grid]") {
    Grid<bounded_extent> grid{{8.5, 1.6, 16.2, 13.1}, 0.1, 0.1};

    CHECK(grid.cols() == 77);
    CHECK(grid.rows() == 115);
}

TEST_CASE("Infinite grid index lookups are correct", "[grid]") {
    Grid<infinite_extent> grid{global, 1.0, 0.5};

    CHECK( grid.get_row(90) == 1 );
    CHECK( grid.get_row(-89.50000001) == 360 );
    CHECK( grid.get_row(-89.5) == 360 );
    CHECK( grid.get_row(-90) == 360 );

    CHECK( grid.get_row(-90.00000001) == 361 );
    CHECK( grid.get_row( 90.00000001) == 0 );

    CHECK( grid.get_column(-180) == 1 );
    CHECK( grid.get_column(-179.000001) == 1 );
    CHECK( grid.get_column(-179) == 2 );
    CHECK( grid.get_column(179) == 360 );
    CHECK( grid.get_column(180) == 360 );

    CHECK( grid.get_column(-180.0000001) == 0 );
    CHECK( grid.get_column( 180.0000001) == 361 );
}

TEST_CASE("Bounded grid index lookups are correct", "[grid]") {
    Grid<bounded_extent> grid{global, 1.0, 0.5};

    CHECK( grid.get_row(90) == 0 );
    CHECK( grid.get_row(-89.50000001) == 359 );
    CHECK( grid.get_row(-89.5) == 359 );
    CHECK( grid.get_row(-90) == 359 );

    CHECK_THROWS( grid.get_row(-90.00000001) );
    CHECK_THROWS( grid.get_row( 90.00000001) );

    CHECK( grid.get_column(-180) == 0 );
    CHECK( grid.get_column(-179.000001) == 0 );
    CHECK( grid.get_column(-179) == 1 );
    CHECK( grid.get_column(179) == 359 );
    CHECK( grid.get_column(180) == 359 );

    CHECK_THROWS( grid.get_column(-180.0000001) );
    CHECK_THROWS( grid.get_column( 180.0000001) );
}

TEST_CASE("Infinite grid shrink works correctly", "[grid]") {
    Grid<infinite_extent> grid1{global, 1, 0.5};

    Grid<infinite_extent> grid2 = grid1.shrink_to_fit({-44.3, -21.4, 18.3, 88.2});

    CHECK( grid2.xmin() == -45 );
    CHECK( grid2.xmax() == 19 );
    CHECK( grid2.ymin() == -21.5 );
    CHECK( grid2.ymax() == 88.5 );
    CHECK( grid2.dx() == grid1.dx() );
    CHECK( grid2.dy() == grid1.dy() );
}

TEST_CASE("Bounded grid shrink works correctly", "[grid]") {
    Grid<bounded_extent> grid1{global, 1, 0.5};

    Grid<bounded_extent> grid2 = grid1.shrink_to_fit({-44.3, -21.4, 18.3, 88.2});

    CHECK( grid2.xmin() == -45 );
    CHECK( grid2.xmax() == 19 );
    CHECK( grid2.ymin() == -21.5 );
    CHECK( grid2.ymax() == 88.5 );
    CHECK( grid2.dx() == grid1.dx() );
    CHECK( grid2.dy() == grid1.dy() );
}

TEST_CASE("Repeated shrink has no effect", "[grid]") {
    Grid<bounded_extent> grid{{-180.5, -90, 180, 90}, 0.1, 0.1};

    Box reduced{ 8.532812500000006, 1.6762207031249972, 16.183398437500017, 13.078515624999994 };

    Grid<bounded_extent> grid2 = grid.shrink_to_fit(reduced);
    Grid<bounded_extent> grid3 = grid2.shrink_to_fit(reduced);
    CHECK( grid2.rows() == grid3.rows() );
    CHECK( grid2.cols() == grid3.cols() );
}

TEST_CASE("Shrink robustness", "[grid]") {
    Grid<bounded_extent> grid{{-180.5, -90, 180, 90}, 0.5, 0.5};

    Box reduced{ -1.0000000000000142, 8.141666666665664,  0.08749999999993818,  9.904166666665645};

    Grid<bounded_extent> grid2 = grid.shrink_to_fit(reduced);

    CHECK(reduced.xmin >= grid2.xmin());
    CHECK(reduced.xmax <= grid2.xmax());
    CHECK(reduced.ymin >= grid2.ymin());
    CHECK(reduced.ymax <= grid2.ymax());
}

TEST_CASE("Shrink robustness (2)", "[grid]") {
    Grid<bounded_extent> grid{{-180.5, -90.5, 180.5, 90.5}, 0.25, 0.25};

    Box reduced{ 129.75833333333242, -1.2541666666666238, 129.7624999999993, -1.2499999999999964 };

    Grid<bounded_extent> grid2 = grid.shrink_to_fit(reduced);

    CHECK(reduced.xmin >= grid2.xmin());
    CHECK(reduced.xmax <= grid2.xmax());
    CHECK(reduced.ymin >= grid2.ymin());
    CHECK(reduced.ymax <= grid2.ymax());
}

TEST_CASE("Cropping", "[grid]") {
    Grid<bounded_extent> grid{{0, 0, 10, 10}, 0.5, 0.5};

    // Cropping with a box larger than the grid's extent has no effect
    CHECK( grid.crop({-100, -100, 100, 100}) == grid );

    // Nor does cropping a grid with its own box
    CHECK( grid.crop(grid.extent()) == grid );

    // Extent of cropped grid is no larger than necessary to contain box
    CHECK( grid.crop({1.8, 2.2, 6.4, 7.5}) == Grid<bounded_extent>{{1.5, 2.0, 6.5, 7.5}, 0.5, 0.5} );

    // But it doesn't expand to cover the box, where the box is larger than the grid's extent
    CHECK( grid.crop({1.8, -2, 11, 7.5}) == Grid<bounded_extent>{{1.5, 0, 10, 7.5}, 0.5, 0.5} );

    // The cropping code hits a special case when the new xmax/ymin falls exactly on a cell boundary
    CHECK( grid.crop({2, 2, 8, 8}) == Grid<bounded_extent>{{2, 2, 8, 8}, 0.5, 0.5} );

    // Cropping to a box outside the extent of the grid produces an empty grid
    CHECK( grid.crop({200, 200, 300, 300}) == Grid<bounded_extent>::make_empty() );
    CHECK( grid.crop({100, 100, 200, 100}) == Grid<bounded_extent>::make_empty() );
}

TEST_CASE("Cropping robustness", "[grid]") {
    Grid<bounded_extent> grid{{-180, -90, 180, 90}, 0.0083333333333333332, 0.0083333333333333332};

    Box b = { 178.60767788357205, 70.782677883572063, 180, 71.542309400770421 };

    auto cropped = grid.crop(b);

    CHECK( grid.extent().contains(cropped.extent()) );
}

TEST_CASE("Cropping robustness (2)") {
    Grid<bounded_extent> grid{{-180, -90, 180, 90}, 0.5, 0.5};
    Box b{179.749999999999972, -18.5833333333333321, 179.999999999999972, -18.5};

    auto cropped = grid.crop(b);

    CHECK( grid.extent().contains(cropped.extent()) );
}

TEST_CASE("Grid compatibility tests", "[grid]") {
    constexpr double tol = 1e-6;

    Grid<bounded_extent> half_degree_global{global, 0.5, 0.5};
    Grid<bounded_extent> one_degree_global{global, 1, 1};
    Grid<bounded_extent> quarter_degree_partial{{-180, -60, 90, 83}, 0.25, 0.25};
    Grid<bounded_extent> nldas{{-125.0, 0.25, -67, 53}, 0.125, 0.125};
    Grid<bounded_extent> tenth_degree_global{global, 0.1, 0.1};
    Grid<bounded_extent> half_degree_offset{{-180.25, -90, -100.25, 50}, 0.5, 0.5};

    CHECK( half_degree_global.compatible_with(one_degree_global, tol) );
    CHECK( quarter_degree_partial.compatible_with(one_degree_global, tol) );
    CHECK( one_degree_global.compatible_with(nldas, tol) );
    CHECK( half_degree_global.compatible_with(tenth_degree_global, tol) );

    CHECK( !quarter_degree_partial.compatible_with(tenth_degree_global, tol) );
    CHECK( !tenth_degree_global.compatible_with(nldas, tol) );
    CHECK( !half_degree_global.compatible_with(half_degree_offset, tol) );
}

TEST_CASE("Grid compatibility, with tolerance", "[grid]") {
    constexpr double tol = 1e-6;

    Grid<bounded_extent> a{{60.525000000000006, 29.308333333333334, 75.166666666666671, 38.491666666666667}, 0.0083333333333333332, 0.0083333333333333332};
    Grid<bounded_extent> b{{60.5, 29, 75.5, 38.5}, 0.5, 0.5};

    CHECK ( a.compatible_with(b, tol) );
    CHECK ( b.compatible_with(a, tol) );
}

TEST_CASE("Grid compatibility with reduced tolerance") {
    constexpr double tol = 1e-3;

    // Examples below are taken from test data used in vignettes for exactextractr
    // The grids are considered compatible before they are pre-cropped to the extent of the input polygons,
    // but not afterwards.
    Grid<bounded_extent> a{{-25.8583333333334, 37.6999999999999, -25.1333333333334, 37.9083333333333}, 1.0/120, 1.0/120};
    Grid<bounded_extent> b{{-25.8550000000072, 37.7029166667142, -25.1345833334558, 37.9095833333478}, 1.0/4800, 1.0/4800};

    CHECK ( a.compatible_with(b, tol) );
    CHECK ( b.compatible_with(a, tol) );
}

TEST_CASE("Grid compatibility (empty grid)", "[grid]") {
    constexpr double tol = 0.0;

    Grid<bounded_extent> half_degree_global{global, 0.5, 0.5};

    CHECK( half_degree_global.compatible_with(Grid<bounded_extent>::make_empty(), tol) );
    CHECK( Grid<bounded_extent>::make_empty().compatible_with(half_degree_global, tol) );
    CHECK( Grid<bounded_extent>::make_empty().compatible_with(Grid<bounded_extent>::make_empty(), tol) );
}

TEST_CASE("Common extent calculation", "[grid]") {
    Grid<bounded_extent> half_degree_global{global, 0.5, 0.5};
    Grid<bounded_extent> nldas{{-125.0, 0.25, -67, 53}, 0.125, 0.125};

    CHECK( nldas.common_grid(half_degree_global) == Grid<bounded_extent>{global, 0.125, 0.125} );
    CHECK( nldas.overlapping_grid(half_degree_global) == nldas );
}

TEST_CASE("Common extent calculation (empty grid)", "[grid]") {
    Grid<bounded_extent> half_degree_global{global, 0.5, 0.5};
    Grid<bounded_extent> empty = Grid<bounded_extent>::make_empty();

    CHECK( half_degree_global.common_grid(empty) == half_degree_global );
    CHECK( half_degree_global.overlapping_grid(empty) == empty );
}

TEST_CASE("Cell center calculations", "[grid]") {
    Grid<bounded_extent> g1{global, 0.5, 0.25};
    Grid<infinite_extent> g2{global, 0.5, 0.25};

    CHECK ( g1.x_for_col(0) == -179.75 );
    CHECK ( g2.x_for_col(1) == -179.75 );

    CHECK ( g1.y_for_row(0) == 89.875 );
    CHECK ( g2.y_for_row(1) == 89.875 );
}

TEST_CASE("Offset calculations", "[grid]") {
    Grid<bounded_extent> g1{global, 0.5, 0.25};
    Grid<bounded_extent> g2{{-170, -90, 180, 88.5}, 0.5, 0.25};

    // Symmetrical; we're expected to already know which grid is positively offset from the other
    CHECK( g1.row_offset(g2) == 6 );
    CHECK( g2.row_offset(g1) == 6 );

    CHECK ( g1.col_offset(g2) == 20 );
    CHECK ( g2.col_offset(g1) == 20 );
}

TEST_CASE("Infinite grid offset calculations", "[grid]") {
    Grid<infinite_extent> g1{global, 0.5, 0.25};
    Grid<infinite_extent> g2{{-170, -90, 180, 88.5}, 0.5, 0.25};

    // Symmetrical; we're expected to already know which grid is positively offset from the other
    CHECK( g1.row_offset(g2) == 6 );
    CHECK( g2.row_offset(g1) == 6 );

    CHECK ( g1.col_offset(g2) == 20 );
    CHECK ( g2.col_offset(g1) == 20 );
}

template<typename T>
static size_t total_subgrid_size(const T& grids) {
    return std::accumulate(
            grids.begin(),
            grids.end(),
            static_cast<size_t>(0),
            [](size_t sum, const Grid<bounded_extent> & g) { return sum + g.size(); });
}

TEST_CASE("Grid subdivision", "[grid]") {
    Grid<bounded_extent> g{{-180, -89.75, 180, 90}, 0.25, 0.25};

    // A row in a global 0.25-degree grid has 1440 cells.
    // With a maximum of 1000 cells per subgrid, we need two subgrids to cover each row.
    auto grids = subdivide(g, 1000);

    CHECK(grids.size() == 2*g.rows());

    // The first subgrid is the maximum size, and the second subgrid gets the leftovers.
    // Technically, you could combine the leftovers from two rows into a subgrid of size 880,
    // and have three subgrids per two rows, but we're not that clever.
    CHECK(grids[0].size() == 1000);
    CHECK(grids[1].size() == 440);
    CHECK(total_subgrid_size(grids) == g.size());

    // If we up the maximum to 3000 cells per subgrid, we can cover two rows per subgrid
    grids = subdivide(g, 3000);

    CHECK(grids.size() == std::ceil(0.5*g.rows()));
    CHECK(grids[0].size() == 2880);
    CHECK(grids[1].size() == 2880);
    CHECK(grids[grids.size()-1].size() == 1440); // leftover single row at the end
    CHECK(total_subgrid_size(grids) == g.size());
}

TEST_CASE("Empty grid subdivision", "[grid]") {
    auto g = Grid<bounded_extent>::make_empty();

    auto grids = subdivide(g, 100);
}
