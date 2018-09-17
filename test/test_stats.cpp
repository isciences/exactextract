#include <cmath>
#include <valarray>

#include "catch.hpp"

#include "grid.h"
#include "raster_cell_intersection.h"
#include "raster_stats.h"
#include "geos_utils.h"

using Catch::Detail::Approx;

namespace exactextract {
    static void init_geos() {
        static bool initialized = false;

        if (!initialized) {
            initGEOS(nullptr, nullptr);
            initialized = true;
        }
    }

    template<typename T>
    void fill_by_row(Raster<T> & r, T start, T dt) {
        T val = start;
        for (size_t i = 0; i < r.rows(); i++) {
            for (size_t j = 0; j < r.cols(); j++) {
                r(i, j) = val;
                val += dt;
            }
        }
    }

    TEST_CASE("Basic float stats") {
        init_geos();

        Grid ex{-1, -1, 4, 4, 1, 1}; // 4x5 grid

        auto g = GEOSGeom_read("POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2.5, 0.5 2.5, 0.5 0.5))");

        Raster<float> areas = raster_cell_intersection(ex, g.get());

        Raster<float> values{{{
          {1, 1, 1, 1, 1},
          {1, 1, 2, 3, 1},
          {1, 4, 5, 6, 1},
          {1, 0, NAN, 7, 1},
          {1, 1, 1, 1, 1}
        }}, -1, -1, 4, 4};

        RasterStats<float> stats{areas, values};

        CHECK( stats.count() ==
               (0.25 + 0.5 + 0.25) +
               (0.50 + 1.0 + 0.50) +
               (0.25 + 0.0 + 0.25)
        );

        CHECK( stats.sum() ==
               (1*0.25 + 2*0.5 + 3*0.25) +
               (4*0.50 + 5*1.0 + 6*0.50) +
               (0*0.25 + 0*0.5 + 7*0.25)
        );

        CHECK( stats.mean() == 13.75f / 3.5f );

        CHECK( stats.mode() == 5 );
        CHECK( stats.minority() == 0 );

        CHECK( stats.min() == 0 );
        CHECK( stats.max() == 7 );

        CHECK( stats.variety() == 8 );
    }

    TEST_CASE("Weighted multiresolution float stats") {
        init_geos();

        Grid ex1{0, 0, 8, 6, 1, 1};
        Grid ex2{0, 0, 8, 6, 2, 2};

        auto g = GEOSGeom_read("POLYGON ((3.5 1.5, 6.5 1.5, 6.5 2.5, 3.5 2.5, 3.5 1.5))");

        Raster<float> areas = raster_cell_intersection(ex1.common_grid(ex2), g.get());
        Raster<float> values{0, 0, 8, 6, 6, 8};
        Raster<float> weights{0, 0, 8, 6, 3, 4};

        fill_by_row<float>(values, 1, 1);
        fill_by_row<float>(weights, 5, 5);

        RasterStats<float> stats{areas, values, weights};

        std::valarray<double> cov_values  = {   28,  29,  30,   31,   36,  37,  38,   39 };
        std::valarray<double> cov_weights = {   30,  35,  35,   40,   50,  55,  55,   60 };
        std::valarray<double> cov_fracs   = { 0.25, 0.5, 0.5, 0.25, 0.25, 0.5, 0.5, 0.25 };

        CHECK( stats.mean() == Approx( (cov_values * cov_weights * cov_fracs).sum() / (cov_weights * cov_fracs).sum() ));
    }

    TEST_CASE("Basic integer stats") {
        init_geos();

        Grid ex{-1, -1, 4, 4, 1, 1}; // 4x5 grid

        auto g = GEOSGeom_read("POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2.5, 0.5 2.5, 0.5 0.5))");

        Raster<float> areas = raster_cell_intersection(ex, g.get());

        int NODATA = -999;

        Raster<int> values{{{
            {1, 1,      1, 1, 1},
            {1, 1,      2, 3, 1},
            {1, 4,      5, 6, 1},
            {1, 0, NODATA, 7, 1},
            {1, 1,      1, 1, 1}
        }}, -1, -1, 4, 4};

        RasterStats<int> stats{areas, values, &NODATA};

        CHECK( stats.count() ==
               (0.25 + 0.5 + 0.25 ) +
               (0.50 + 1.0 + 0.50 ) +
               (0.25 + 0.0 + 0.25)
        );

        CHECK( stats.sum() ==
               (1*0.25 + 2*0.5 + 3*0.25) +
               (4*0.50 + 5*1.0 + 6*0.50) +
               (0*0.25 + 0*0.5 + 7*0.25)
        );

        CHECK( stats.mean() == 13.75f / 3.5f );

        CHECK( stats.mode() == 5 );
        CHECK( stats.minority() == 0 );

        CHECK( stats.min() == 0 );
        CHECK( stats.max() == 7 );

        CHECK( stats.variety() == 8 );
    }
}
