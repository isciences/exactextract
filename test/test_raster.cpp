#include <limits>

#include "catch.hpp"

#include "grid.h"
#include "matrix.h"
#include "raster.h"

using namespace exactextract;

template<typename T>
void
fill_with_squares(Raster<T>& r)
{
    for (size_t i = 0; i < r.rows(); i++) {
        for (size_t j = 0; j < r.cols(); j++) {
            r(i, j) = i * j;
        }
    }
}

template<typename T>
void
fill_sequential(Raster<T>& r)
{
    for (size_t i = 0; i < r.rows(); i++) {
        for (size_t j = 0; j < r.cols(); j++) {
            r(i, j) = i * r.cols() + j;
        }
    }
}

template<typename T>
static void
print(const AbstractRaster<T>& r)
{
    for (size_t i = 0; i < r.rows(); i++) {
        for (size_t j = 0; j < r.cols(); j++) {
            std::cout << r(i, j) << '\t';
        }
        std::cout << std::endl;
    }
}

TEST_CASE("Constructing a Raster")
{
    Raster<float> r{ { -180, -90, 180, 90 }, 180, 360 };

    fill_with_squares(r);

    CHECK(r.rows() == 180);
    CHECK(r.cols() == 360);
    CHECK(r.xres() == 1.0);
    CHECK(r.yres() == 1.0);
    CHECK(r.xmin() == -180);
    CHECK(r.xmax() == 180);
    CHECK(r.ymin() == -90);
    CHECK(r.ymax() == 90);

    bool all_equal = true;
    for (size_t i = 0; i < r.rows(); i++) {
        for (size_t j = 0; j < r.cols(); j++) {
            if (r(i, j) != i * j)
                all_equal = false;
        }
    }

    CHECK(all_equal);
};

TEST_CASE("Rasters are unequal when their values differ")
{
    Raster<float> r1{ { 0, 0, 1, 1 }, 10, 10 };
    Raster<float> r2{ { 0, 0, 1, 1 }, 10, 10 };

    fill_with_squares(r1);
    fill_with_squares(r2);

    CHECK(r1 == r2);
    CHECK(!(r1 != r2));

    r2(1, 3) = -5;

    CHECK(!(r1 == r2));
    CHECK(r1 != r2);
}

TEST_CASE("Can access raster cells by value or by reference")
{
    Raster<float> r{ { 0, 0, 1, 1 }, 10, 10 };
    fill_sequential(r);

    float orig = r(2, 2);
    float& cell = r(2, 2);
    cell *= 2;

    CHECK(r(2, 2) == orig * 2);
}

TEST_CASE("Rasters are unequal when their extents differ")
{
    Raster<float> r1{ { 0, 0, 1, 1 }, 10, 10 };
    Raster<float> r2{ { 0, 0, 1, 10 }, 10, 10 };

    fill_with_squares(r1);
    fill_with_squares(r2);

    CHECK(r1 != r2);
}

TEST_CASE("Rasters are equal if their NODATA values differ, only long as the NODATA value is never used (1)")
{
    Raster<int> r1{ { 0, 0, 1, 1 }, 10, 10 };
    Raster<int> r2{ { 0, 0, 1, 1 }, 10, 10 };

    r1.set_nodata(999);

    fill_with_squares(r1);
    fill_with_squares(r2);

    CHECK(r1.has_nodata());
    CHECK(!r2.has_nodata());

    CHECK(r1 == r2);

    r1.set_nodata(25);

    CHECK(r1 != r2);

    r2.set_nodata(25);

    CHECK(r1 == r2);
}

TEST_CASE("Rasters are equal if their NODATA values differ, only long as the NODATA value is never used (2)")
{
    Raster<int> r1{ { 0, 0, 1, 1 }, 10, 10 };
    Raster<int> r2{ { 0, 0, 1, 1 }, 10, 10 };

    fill_with_squares(r1);
    fill_with_squares(r2);

    CHECK(r1 == r2);

    r2.set_nodata(25);

    CHECK(r1 != r2);

    r1.set_nodata(25);

    CHECK(r1 == r2);
}

TEST_CASE("Creating a scaled view")
{
    Raster<float> r{ { 0, 0, 10, 10 }, 10, 10 };
    Grid<bounded_extent> ex{ { 0, 0, 10, 10 }, 0.1, 0.1 };

    fill_with_squares(r);

    RasterView<float> rv(r, ex);

    CHECK(rv.xmin() == 0);
    CHECK(rv.ymin() == 0);
    CHECK(rv.xmax() == 10);
    CHECK(rv.ymax() == 10);
    CHECK(rv.rows() == 100);
    CHECK(rv.cols() == 100);

    bool all_equal = true;
    for (size_t i = 0; i < rv.rows(); i++) {
        for (size_t j = 0; j < rv.cols(); j++) {
            if (rv(i, j) != (int(i / 10)) * (int(j / 10)))
                all_equal = false;
        }
    }

    CHECK(all_equal);
}

TEST_CASE("Creating a shifted view")
{
    Raster<float> r{ { 0, 0, 10, 10 }, 10, 10 };
    Box clipped = { 2, 3, 5, 8 };
    Grid<bounded_extent> ex{ clipped, 1, 1 };

    fill_with_squares(r);

    RasterView<float> rv(r, ex);

    CHECK(rv.xmin() == 2);
    CHECK(rv.ymin() == 3);
    CHECK(rv.xmax() == 5);
    CHECK(rv.ymax() == 8);
    CHECK(rv.rows() == 5);
    CHECK(rv.cols() == 3);
    CHECK(rv.xres() == 1);
    CHECK(rv.yres() == 1);

    Matrix<float> expected_values{ { { 4, 6, 8 },
                                     { 6, 9, 12 },
                                     { 8, 12, 16 },
                                     { 10, 15, 20 },
                                     { 12, 18, 24 } } };

    Raster<float> expected{ std::move(expected_values), clipped };

    CHECK(rv == expected);
}

TEST_CASE("Creating a scaled and shifted view")
{
    Raster<float> r{ { 0, 0, 10, 10 }, 10, 10 };
    Box clipped = { 2.5, 3, 5, 8.5 };
    Grid<bounded_extent> ex{ clipped, 0.5, 0.5 };

    fill_with_squares(r);

    RasterView<float> rv(r, ex);

    CHECK(rv.xmin() == 2.5);
    CHECK(rv.ymin() == 3.0);
    CHECK(rv.xmax() == 5.0);
    CHECK(rv.ymax() == 8.5);
    CHECK(rv.rows() == 11);
    CHECK(rv.cols() == 5);
    CHECK(rv.xres() == 0.5);
    CHECK(rv.yres() == 0.5);

    Matrix<float> expected_values{ { { 2, 3, 3, 4, 4 },
                                     { 4, 6, 6, 8, 8 },
                                     { 4, 6, 6, 8, 8 },
                                     { 6, 9, 9, 12, 12 },
                                     { 6, 9, 9, 12, 12 },
                                     { 8, 12, 12, 16, 16 },
                                     { 8, 12, 12, 16, 16 },
                                     { 10, 15, 15, 20, 20 },
                                     { 10, 15, 15, 20, 20 },
                                     { 12, 18, 18, 24, 24 },
                                     { 12, 18, 18, 24, 24 } } };

    Raster<float> expected{ std::move(expected_values), clipped };

    CHECK(rv == expected);
}

TEST_CASE("Creating a scaled and shifted view (greater extent)")
{
    Raster<float> r{ { 0, 0, 10, 10 }, 10, 10 };
    Box expanded{ 2.5, 8.5, 4, 11 };
    Grid<bounded_extent> ex{ expanded, 0.5, 0.5 };

    fill_with_squares(r);

    RasterView<float> rv(r, ex);

    CHECK(rv.xmin() == 2.5);
    CHECK(rv.ymin() == 8.5);
    CHECK(rv.xmax() == 4.0);
    CHECK(rv.ymax() == 11);
    CHECK(rv.rows() == 5);
    CHECK(rv.cols() == 3);
    CHECK(rv.xres() == 0.5);
    CHECK(rv.yres() == 0.5);

    float nan = std::numeric_limits<float>::quiet_NaN();

    Matrix<float> expected_values{ { { nan, nan, nan },
                                     { nan, nan, nan },
                                     { 0, 0, 0 },
                                     { 0, 0, 0 },
                                     { 2, 3, 3 } } };

    Raster<float> expected{ std::move(expected_values), expanded };

    CHECK(rv == expected);
}

TEST_CASE("Creating a shifted view (robustness)")
{
    Box rast_box{ -130.76666666666947, -25.083333333335318, -124.77500000000313, -23.916666666668718 };
    Box view_box{ -130.75833333333614, -25.083333333335318, -124.77500000000313, -23.916666666668718 };
    // Extent of view starts one cell of the right of the original raster, so column N in the view should
    // correspond to column N+1 of the original.

    double res = 0.0083333333333328596;
    Raster<int> rast{ Grid<bounded_extent>{ rast_box, res, res } };
    fill_sequential(rast);

    RasterView<int> rv{ rast, Grid<bounded_extent>{ view_box, res, res } };

    CHECK(rv(5, 5) == rast(5, 6));
}

TEST_CASE("Creating a scaled view (robustness)")
{
    Box rast_box{ -180, -90, 180, 90 };

    double rast_res = 0.5 - 1e-12;
    double view_res = 2.5 / 60;

    Raster<int> rast{ Grid<bounded_extent>{ rast_box, rast_res, rast_res } };
    fill_sequential(rast);
    RasterView<int> rv{ rast, Grid<bounded_extent>{ rast_box, view_res, view_res } };

    CHECK(rv.rows() == 4320);

    size_t i = 2015;
    for (size_t j = 0; j < rv.cols(); j++) {
        CHECK(rv(i, j) == rast(i / 12, j / 12));
    }
}

TEST_CASE("Empty view")
{
    Box rast_box{ 10, 0, 10, 10 };
    double res = 1.0;

    Raster<double> rast{ Grid<bounded_extent>{ rast_box, res, res } };

    RasterView<double> rv{ rast, Grid<bounded_extent>::make_empty() };

    CHECK(rv.rows() == 0);
    CHECK(rv.cols() == 0);
}

TEST_CASE("View on empty raster")
{
    Raster<double> rast{ Grid<bounded_extent>::make_empty() };

    Box rast_box{ -10, -10, 10, 10 };
    double res = 0.5;
    Grid<bounded_extent> view_grid{ rast_box, res, res };

    RasterView<double> rv{ rast, view_grid };

    CHECK(24 < rv.rows());
    CHECK(24 < rv.cols());

    CHECK(std::isnan(rv(24, 24)));
}

TEST_CASE("Expanded view")
{
    Box rast_box{ 5, 10, 20, 20 };
    Box view_box{ 0, 0, 30, 30 };

    double res = 1;
    Raster<double> rast{ Grid<bounded_extent>{ rast_box, res, res } };
    fill_sequential(rast);

    RasterView<double> rv{ rast, Grid<bounded_extent>{ view_box, res, res } };

    // Check 3 points outside UL corner
    CHECK(std::isnan(rv(9, 4)));
    CHECK(std::isnan(rv(10, 4)));
    CHECK(std::isnan(rv(9, 5)));

    // Check point just inside UL corner
    CHECK(rv(10, 5) == rast(0, 0));

    // Check 3 points outside UR corner
    CHECK(std::isnan(rv(9, 19)));
    CHECK(std::isnan(rv(10, 20)));
    CHECK(std::isnan(rv(9, 20)));

    // Check point just inside UR corner
    CHECK(rv(10, 19) == rast(0, 14));

    // Check 3 points just outside LL corner
    CHECK(std::isnan(rv(19, 4)));
    CHECK(std::isnan(rv(20, 4)));
    CHECK(std::isnan(rv(20, 5)));

    // Check point just inside LL corner
    CHECK(rv(19, 5) == rast(9, 0));

    // Check 3 points just outside LR corner
    CHECK(std::isnan(rv(20, 20)));
    CHECK(std::isnan(rv(20, 19)));
    CHECK(std::isnan(rv(19, 20)));

    // Check point just inside LR corner
    CHECK(rv(19, 19) == rast(9, 14));
}

TEST_CASE("Get method accesses value and tells us if it was defined")
{
    float nan = std::numeric_limits<float>::quiet_NaN();

    Raster<float> r{
        Matrix<float>{ { { 1, -999 },
                         { nan, 7 } } },
        Box{ 0, 0, 2, 2 }
    };
    r.set_nodata(-999);

    float f{ 0 };
    CHECK(r.get(0, 0, f));
    CHECK(f == 1);
    CHECK(!r.get(0, 1, f));
    CHECK(!r.get(1, 0, f));
    CHECK(r.get(1, 1, f));
    CHECK(f == 7);
}