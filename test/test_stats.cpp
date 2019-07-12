#include <cmath>
#include <valarray>

#include "catch.hpp"

#include "grid.h"
#include "raster_cell_intersection.h"
#include "raster_stats.h"
#include "geos_utils.h"

using Catch::Detail::Approx;

namespace exactextract {
    static GEOSContextHandle_t init_geos() {
        static GEOSContextHandle_t context = nullptr;

        if (context == nullptr) {
            context = initGEOS_r(nullptr, nullptr);
        }

        return context;
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
        GEOSContextHandle_t context = init_geos();

        Box extent{-1, -1, 4, 4};
        Grid<bounded_extent> ex{extent, 1, 1}; // 4x5 grid

        auto g = GEOSGeom_read_r(context, "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2.5, 0.5 2.5, 0.5 0.5))");

        Raster<float> areas = raster_cell_intersection(ex, context, g.get());

        Raster<float> values{Matrix<float>{{
          {1, 1, 1, 1, 1},
          {1, 1, 2, 3, 1},
          {1, 4, 5, 6, 1},
          {1, 0, NAN, 7, 1},
          {1, 1, 1, 1, 1}
        }}, extent};

        RasterStats<float> stats{true};
        stats.process(areas, values);

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
        GEOSContextHandle_t context = init_geos();

        Box extent { 0, 0, 8, 6 };

        Grid<bounded_extent> ex1{extent, 1, 1};
        Grid<bounded_extent> ex2{extent, 2, 2};

        auto g = GEOSGeom_read_r(context, "POLYGON ((3.5 1.5, 6.5 1.5, 6.5 2.5, 3.5 2.5, 3.5 1.5))");

        Raster<float> areas = raster_cell_intersection(ex1.common_grid(ex2), context, g.get());
        Raster<float> values{extent, 6, 8};
        Raster<float> weights{extent, 3, 4};

        fill_by_row<float>(values, 1, 1);
        fill_by_row<float>(weights, 5, 5);

        RasterStats<float> stats(false);
        stats.process(areas, values, weights);

        std::valarray<double> cov_values  = {   28,  29,  30,   31,   36,  37,  38,   39 };
        std::valarray<double> cov_weights = {   30,  35,  35,   40,   50,  55,  55,   60 };
        std::valarray<double> cov_fracs   = { 0.25, 0.5, 0.5, 0.25, 0.25, 0.5, 0.5, 0.25 };

        CHECK( stats.weighted_mean() == Approx( (cov_values * cov_weights * cov_fracs).sum() / (cov_weights * cov_fracs).sum() ));
        CHECK( stats.mean() == Approx( (cov_values *  cov_fracs).sum() / cov_fracs.sum() ));

        CHECK( stats.weighted_fraction() == Approx( (cov_values*cov_weights*cov_fracs).sum() / (cov_values*cov_fracs).sum() ));
    }

    TEST_CASE("Basic integer stats") {
        GEOSContextHandle_t context = init_geos();

        Box extent{-1, -1, 4, 4};
        Grid<bounded_extent> ex{extent, 1, 1}; // 4x5 grid

        auto g = GEOSGeom_read_r(context, "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2.5, 0.5 2.5, 0.5 0.5))");

        Raster<float> areas = raster_cell_intersection(ex, context, g.get());

        int NODATA = -999;

        Raster<int> values{Matrix<int>{{
            {1, 1,      1, 1, 1},
            {1, 1,      2, 3, 1},
            {1, 4,      5, 6, 1},
            {1, 0, NODATA, 7, 1},
            {1, 1,      1, 1, 1}
        }}, extent};
        values.set_nodata(NODATA);

        RasterStats<int> stats{true};
        stats.process(areas, values);

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

    TEST_CASE("Missing data handling") {
        GEOSContextHandle_t context = init_geos();

        Box extent{0, 0, 2, 2};
        Grid<bounded_extent> ex{extent, 1, 1}; // 2x2 grid

        auto g = GEOSGeom_read_r(context, "POLYGON ((0.5 0.5, 1.5 0.5, 1.5 1.5, 0.5 1.5, 0.5 0.5))");

        Raster<float> areas = raster_cell_intersection(ex, context, g.get());

        // Polygon covers 25% of each of the four cells
        for (size_t i = 0; i < areas.rows(); i++) {
            for (size_t j = 0; j < areas.cols(); j++) {
                CHECK( areas(i, j) == 0.25f );
            }
        }

        Raster<float> all_values_missing{Matrix<float>{{
            {NAN, NAN},
            {NAN, NAN}
        }}, extent};

        Raster<float> all_values_defined{Matrix<float>{{
            {1, 2},
            {3, 4},
        }}, extent};

        Raster<float> some_values_defined{Matrix<float>{{
            {1,     2},
            {NAN, NAN}
        }}, extent};

        // All values missing, no weights provided
        // Example application: land cover on an island not covered by dataset
        {
            RasterStats<float> stats(false);
            stats.process(areas, all_values_missing);

            CHECK( stats.count() == 0 );
            CHECK( stats.sum() == 0 );
            CHECK( std::isnan(stats.min()) );
            CHECK( std::isnan(stats.max()) );
            CHECK( std::isnan(stats.mean()) );
            CHECK( std::isnan(stats.mode()) );
            CHECK( std::isnan(stats.minority()) );
            CHECK( stats.variety() == 0 );
            CHECK( stats.weighted_count() == stats.count() );
            CHECK( stats.weighted_sum() == stats.sum() );
            CHECK( std::isnan(stats.weighted_mean()) );
        }

        // All values defined, no weights defined
        // Example application: precipitation over polygon in the middle of continent
        {
            RasterStats<float> stats{true};
            stats.process(areas, all_values_defined);

            CHECK( stats.count() == 1.0f );
            CHECK( stats.sum() == 2.5f );
            CHECK( stats.min() == 1.0f );
            CHECK( stats.max() == 4.0f );
            CHECK( stats.mean() == 2.5f );
            CHECK( stats.mode() == 4.0f );
            CHECK( stats.minority() == 1.0f );
            CHECK( stats.weighted_count() == stats.count() );
            CHECK( stats.weighted_sum() == stats.sum() );
            CHECK( stats.weighted_mean() == stats.mean() );
        }

        // Some values defined, no weights provided
        // Example application: precipitation at edge of continent
        {
            RasterStats<float> stats{true};
            stats.process(areas, some_values_defined);

            CHECK( stats.count() == 0.5f );
            CHECK( stats.sum() == 0.75f );
            CHECK( stats.min() == 1.0f );
            CHECK( stats.max() == 2.0f );
            CHECK( stats.mean() > 0.5f );
            CHECK( stats.mode() == 2.0f );
            CHECK( stats.minority() == 1.0f );
            CHECK( stats.weighted_count() == stats.count() );
            CHECK( stats.weighted_sum() == stats.sum() );
            CHECK( stats.weighted_mean() == stats.mean() );
        }

        // No values defined, all weights defined
        // Example: population-weighted temperature in dataset covered by pop but without temperature data
        {
            RasterStats<float> stats{true};
            stats.process(areas, all_values_missing, all_values_defined);

            CHECK( stats.count() == 0 );
            CHECK( stats.sum() == 0 );
            CHECK( std::isnan(stats.min()) );
            CHECK( std::isnan(stats.max()) );
            CHECK( std::isnan(stats.mean()) );
            CHECK( stats.weighted_count() == stats.count() );
            CHECK( stats.weighted_sum() == stats.sum() );
            CHECK( std::isnan(stats.weighted_mean()) );
        }

        // All values defined, no weights defined
        // Example: population-weighted temperature in polygon covered by temperature but without pop data
        {
            RasterStats<float> stats{true};
            stats.process(areas, all_values_defined, all_values_missing);

            CHECK( stats.count() == 1.0f );
            CHECK( stats.sum() == 2.5f );
            CHECK( stats.min() == 1.0f );
            CHECK( stats.max() == 4.0f );
            CHECK( stats.mean() == 2.5f );
            CHECK( std::isnan(stats.weighted_count()) );
            CHECK( std::isnan(stats.weighted_sum()) );
            CHECK( std::isnan(stats.weighted_mean()) );
        }

        // All values defined, some weights defined
        {
            RasterStats<float> stats{true};
            stats.process(areas, all_values_defined, some_values_defined);

            CHECK( stats.count() == 1.0f );
            CHECK( stats.sum() == 2.5f );
            CHECK( stats.min() == 1.0f );
            CHECK( stats.max() == 4.0f );
            CHECK( stats.mean() == 2.5f );
            CHECK( std::isnan(stats.weighted_count()) );
            CHECK( std::isnan(stats.weighted_sum()) );
            CHECK( std::isnan(stats.weighted_mean()) );
        }
    }
}
