#include <limits>

#include "catch.hpp"

#include "grid.h"
#include "matrix.h"
#include "raster.h"

using namespace exactextract;

template<typename T>
void fill_with_squares(Raster<T> & r) {
    for (size_t i = 0; i < r.rows(); i++) {
        for (size_t j = 0; j < r.cols(); j++) {
            r(i, j) = i*j;
        }
    }
}

template<typename T>
static void print(const AbstractRaster<T> & r) {
    for (size_t i = 0; i < r.rows(); i++) {
        for (size_t j = 0; j < r.cols(); j++) {
            std::cout << r(i, j) << '\t';
        }
        std::cout << std::endl;
    }
}

TEST_CASE("Constructing a Raster" ) {
    Raster<float> r{{-180, -90, 180, 90}, 180, 360};

    fill_with_squares(r);

    CHECK ( r.rows() == 180 );
    CHECK ( r.cols() == 360 );
    CHECK ( r.xres() == 1.0 );
    CHECK ( r.yres() == 1.0 );
    CHECK ( r.xmin() == -180 );
    CHECK ( r.xmax() == 180 );
    CHECK ( r.ymin() == -90 );
    CHECK ( r.ymax() == 90 );

    bool all_equal = true;
    for (size_t i = 0; i < r.rows(); i++) {
        for (size_t j = 0; j < r.cols(); j++) {
            if (r(i, j) != i*j)
                all_equal = false;
        }
    }

    CHECK (all_equal);
};

TEST_CASE("Creating a scaled view") {
    Raster<float> r{{0, 0, 10, 10}, 10, 10};
    Grid<bounded_extent> ex{{0, 0, 10, 10}, 0.1, 0.1};

    fill_with_squares(r);

    RasterView<float> rv(r, ex);

    CHECK ( rv.xmin() == 0 );
    CHECK ( rv.ymin() == 0 );
    CHECK ( rv.xmax() == 10 );
    CHECK ( rv.ymax() == 10 );
    CHECK ( rv.rows() == 100 );
    CHECK ( rv.cols() == 100 );

    bool all_equal = true;
    for (size_t i = 0; i < rv.rows(); i++) {
        for (size_t j = 0; j < rv.cols(); j++) {
            if (rv(i, j) != (int (i/10))*(int (j/10)))
                all_equal = false;
        }
    }

    CHECK (all_equal);
}

TEST_CASE("Creating a shifted view") {
    Raster<float> r{{0, 0, 10, 10}, 10, 10};
    Box clipped = {2, 3, 5, 8};
    Grid<bounded_extent> ex{clipped, 1, 1};

    fill_with_squares(r);

    RasterView<float> rv(r, ex);

    CHECK ( rv.xmin() == 2 );
    CHECK ( rv.ymin() == 3 );
    CHECK ( rv.xmax() == 5 );
    CHECK ( rv.ymax() == 8 );
    CHECK ( rv.rows() == 5 );
    CHECK ( rv.cols() == 3 );
    CHECK ( rv.xres() == 1 );
    CHECK ( rv.yres() == 1 );

    Matrix<float> expected_values{{
            { 4, 6, 8 },
            { 6, 9, 12},
            { 8, 12, 16},
            { 10, 15, 20},
            { 12, 18, 24}
    }};

    Raster<float> expected{std::move(expected_values), clipped};

    CHECK (rv == expected);
}

TEST_CASE("Creating a scaled and shifted view") {
    Raster<float> r{{0, 0, 10, 10}, 10, 10};
    Box clipped = {2.5, 3, 5, 8.5};
    Grid<bounded_extent> ex{clipped, 0.5, 0.5};

    fill_with_squares(r);

    RasterView<float> rv(r, ex);

    CHECK ( rv.xmin() == 2.5 );
    CHECK ( rv.ymin() == 3.0 );
    CHECK ( rv.xmax() == 5.0 );
    CHECK ( rv.ymax() == 8.5 );
    CHECK ( rv.rows() == 11 );
    CHECK ( rv.cols() == 5 );
    CHECK ( rv.xres() == 0.5 );
    CHECK ( rv.yres() == 0.5 );

    Matrix<float> expected_values{{
          { 2,  3,   3,  4,  4 },
          { 4,  6,   6,  8,  8 },
          { 4,  6,   6,  8,  8 },
          { 6,  9,   9, 12, 12 },
          { 6,  9,   9, 12, 12 },
          { 8,  12, 12, 16, 16 },
          { 8,  12, 12, 16, 16 },
          { 10, 15, 15, 20, 20 },
          { 10, 15, 15, 20, 20 },
          { 12, 18, 18, 24, 24 },
          { 12, 18, 18, 24, 24 }
  }};

    Raster<float> expected{std::move(expected_values), clipped};

    CHECK (rv == expected);
}

TEST_CASE("Creating a scaled and shifted view (greater extent)") {
    Raster<float> r{{0, 0, 10, 10}, 10, 10};
    Box expanded { 2.5, 8.5, 4, 11 };
    Grid<bounded_extent> ex{expanded, 0.5, 0.5};

    fill_with_squares(r);

    RasterView<float> rv(r, ex);

    CHECK ( rv.xmin() == 2.5 );
    CHECK ( rv.ymin() == 8.5 );
    CHECK ( rv.xmax() == 4.0 );
    CHECK ( rv.ymax() == 11 );
    CHECK ( rv.rows() == 5 );
    CHECK ( rv.cols() == 3 );
    CHECK ( rv.xres() == 0.5 );
    CHECK ( rv.yres() == 0.5 );

    float nan = std::numeric_limits<float>::quiet_NaN();

    Matrix<float> expected_values{{
          { nan, nan, nan },
          { nan, nan, nan },
          {   0,   0,   0 },
          {   0,   0,   0 },
          {   2,   3,   3 }
    }};

    Raster<float> expected{std::move(expected_values), expanded};

    CHECK (rv == expected );
}
