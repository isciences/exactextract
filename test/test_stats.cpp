#include <cmath>

#include "catch.hpp"

#include "extent.h"
#include "raster_cell_intersection.h"
#include "raster_stats.h"
#include "geos_utils.h"

namespace exactextract {
    static void init_geos() {
        static bool initialized = false;

        if (!initialized) {
            initGEOS(nullptr, nullptr);
            initialized = true;
        }
    }

    TEST_CASE("Basic float stats") {
        init_geos();

        Extent ex{-1, -1, 4, 4, 1, 1}; // 4x5 grid

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

    TEST_CASE("Basic integer stats") {
        init_geos();

        Extent ex{-1, -1, 4, 4, 1, 1}; // 4x5 grid

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
