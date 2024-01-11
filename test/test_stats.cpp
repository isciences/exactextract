#include <cmath>
#include <valarray>

#include "catch.hpp"

#include "geos_utils.h"
#include "grid.h"
#include "raster_cell_intersection.h"
#include "raster_stats.h"
#include "variance.h"
#include "weighted_quantiles.h"

using Catch::Detail::Approx;

namespace exactextract {
static GEOSContextHandle_t
init_geos()
{
    static GEOSContextHandle_t context = nullptr;

    if (context == nullptr) {
        context = initGEOS_r(nullptr, nullptr);
    }

    return context;
}

template<typename T>
void
fill_by_row(Raster<T>& r, T start, T dt)
{
    T val = start;
    for (size_t i = 0; i < r.rows(); i++) {
        for (size_t j = 0; j < r.cols(); j++) {
            r(i, j) = val;
            val += dt;
        }
    }
}

template<typename T>
T
nodata_test_value();

template<>
float
nodata_test_value()
{
    return NAN;
}

template<>
double
nodata_test_value()
{
    return -3.2e38;
}

template<>
int
nodata_test_value()
{
    return -999;
}

TEMPLATE_TEST_CASE("Unweighted stats", "[stats]", float, double, int)
{
    GEOSContextHandle_t context = init_geos();

    Box extent{ -1, -1, 4, 4 };
    Grid<bounded_extent> ex{ extent, 1, 1 }; // 4x5 grid

    auto g = GEOSGeom_read_r(context, "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2.5, 0.5 2.5, 0.5 0.5))");

    Raster<float> areas = raster_cell_intersection(ex, context, g.get());

    auto NA = nodata_test_value<TestType>();
    Raster<TestType> values{ Matrix<TestType>{ { { 1, 1, 1, 1, 1 },
                                                 { 1, 1, 2, 3, 1 },
                                                 { 1, 4, 5, 6, 1 },
                                                 { 1, 0, NA, 7, 1 },
                                                 { 1, 1, 1, 1, 1 } } },
                             extent };
    values.set_nodata(NA);

    RasterStatsOptions opts;
    opts.store_histogram = true;
    RasterStats<TestType> stats{ opts };
    stats.process(areas, values);

    CHECK(stats.count() ==
          (0.25 + 0.5 + 0.25) +
            (0.50 + 1.0 + 0.50) +
            (0.25 + 0.0 + 0.25));

    CHECK(stats.sum() ==
          (1 * 0.25 + 2 * 0.5 + 3 * 0.25) +
            (4 * 0.50 + 5 * 1.0 + 6 * 0.50) +
            (0 * 0.25 + 0 * 0.5 + 7 * 0.25));

    CHECK(stats.mean() == 13.75f / 3.5f);

    CHECK(stats.mode() == 5);
    CHECK(stats.minority() == 0);

    CHECK(stats.min() == 0);
    CHECK(stats.max() == 7);

    CHECK(stats.variety() == 8);
}

TEMPLATE_TEST_CASE("Weighted multiresolution stats", "[stats]", float, double, int)
{
    GEOSContextHandle_t context = init_geos();

    Box extent{ 0, 0, 8, 6 };

    Grid<bounded_extent> ex1{ extent, 1, 1 };
    Grid<bounded_extent> ex2{ extent, 2, 2 };

    auto g = GEOSGeom_read_r(context, "POLYGON ((3.5 1.5, 6.5 1.5, 6.5 2.5, 3.5 2.5, 3.5 1.5))");

    Raster<float> areas = raster_cell_intersection(ex1.common_grid(ex2), context, g.get());
    Raster<TestType> values{ extent, 6, 8 };
    Raster<TestType> weights{ extent, 3, 4 };

    fill_by_row<TestType>(values, 1, 1);
    fill_by_row<TestType>(weights, 5, 5);

    RasterStats<TestType> stats;
    stats.process(areas, values, weights);

    std::valarray<double> cov_values = { 28, 29, 30, 31, 36, 37, 38, 39 };
    std::valarray<double> cov_weights = { 30, 35, 35, 40, 50, 55, 55, 60 };
    std::valarray<double> cov_fracs = { 0.25, 0.5, 0.5, 0.25, 0.25, 0.5, 0.5, 0.25 };

    CHECK(stats.weighted_mean() == Approx((cov_values * cov_weights * cov_fracs).sum() / (cov_weights * cov_fracs).sum()));
    CHECK(stats.mean() == Approx((cov_values * cov_fracs).sum() / cov_fracs.sum()));

    CHECK(stats.weighted_fraction() == Approx((cov_values * cov_weights * cov_fracs).sum() / (cov_values * cov_fracs).sum()));
}

TEMPLATE_TEST_CASE("Missing data handling", "[stats]", float, double, int)
{
    GEOSContextHandle_t context = init_geos();

    Box extent{ 0, 0, 2, 2 };
    Grid<bounded_extent> ex{ extent, 1, 1 }; // 2x2 grid

    auto g = GEOSGeom_read_r(context, "POLYGON ((0.5 0.5, 1.5 0.5, 1.5 1.5, 0.5 1.5, 0.5 0.5))");

    Raster<float> areas = raster_cell_intersection(ex, context, g.get());

    // Polygon covers 25% of each of the four cells
    for (size_t i = 0; i < areas.rows(); i++) {
        for (size_t j = 0; j < areas.cols(); j++) {
            CHECK(areas(i, j) == 0.25f);
        }
    }

    auto NA = nodata_test_value<TestType>();

    Raster<TestType> all_values_missing{ Matrix<TestType>{ { { NA, NA },
                                                             { NA, NA } } },
                                         extent };
    all_values_missing.set_nodata(NA);

    Raster<TestType> all_values_defined{ Matrix<TestType>{ {
                                           { 1, 2 },
                                           { 3, 4 },
                                         } },
                                         extent };
    all_values_defined.set_nodata(NA);

    Raster<TestType> some_values_defined{ Matrix<TestType>{ { { 1, 2 },
                                                              { NA, NA } } },
                                          extent };
    some_values_defined.set_nodata(NA);

    SECTION("All values missing, no weights provided")
    {
        // Example application: land cover on an island not covered by dataset
        RasterStats<TestType> stats;
        stats.process(areas, all_values_missing);

        CHECK(stats.count() == 0);
        CHECK(stats.sum() == 0);
        CHECK(!stats.min().has_value());
        CHECK(!stats.max().has_value());
        CHECK(std::isnan(stats.mean()));
        CHECK(std::isnan(stats.variance()));
        CHECK(std::isnan(stats.stdev()));
        CHECK(std::isnan(stats.weighted_variance()));
        CHECK(std::isnan(stats.weighted_stdev()));
        CHECK(std::isnan(stats.coefficient_of_variation()));
        CHECK(!stats.mode().has_value());
        CHECK(!stats.minority().has_value());
        CHECK(stats.variety() == 0);
        CHECK(stats.weighted_count() == stats.count());
        CHECK(stats.weighted_sum() == stats.sum());
        CHECK(std::isnan(stats.weighted_mean()));
        CHECK(!stats.min_xy().has_value());
        CHECK(!stats.max_xy().has_value());
    }

    SECTION("All values defined, no weights provided")
    {
        // Example application: precipitation over polygon in the middle of continent
        RasterStatsOptions opts;
        opts.store_histogram = true;
        opts.calc_variance = true;
        RasterStats<TestType> stats{ opts };
        stats.process(areas, all_values_defined);

        CHECK(stats.count() == 1.0f);
        CHECK(stats.sum() == 2.5f);
        CHECK(stats.min() == 1.0f);
        CHECK(stats.max() == 4.0f);
        CHECK(stats.mean() == 2.5f);
        CHECK(stats.mode() == 4.0f);
        CHECK(stats.minority() == 1.0f);
        CHECK(stats.variance() == 1.25f);
        CHECK(stats.stdev() == 1.118034f);
        CHECK(stats.weighted_variance() == stats.variance());
        CHECK(stats.weighted_stdev() == stats.stdev());
        CHECK(stats.coefficient_of_variation() == 0.4472136f);
        CHECK(stats.weighted_count() == stats.count());
        CHECK(stats.weighted_sum() == stats.sum());
        CHECK(stats.weighted_mean() == stats.mean());
    }

    SECTION("Some values defined, no weights provided")
    {
        // Example application: precipitation at edge of continent
        RasterStatsOptions opts;
        opts.calc_variance = true;
        opts.store_histogram = true;
        RasterStats<TestType> stats{ opts };
        stats.process(areas, some_values_defined);

        CHECK(stats.count() == 0.5f);
        CHECK(stats.sum() == 0.75f);
        CHECK(stats.min() == 1.0f);
        CHECK(stats.max() == 2.0f);
        CHECK(stats.mean() == 1.5f);
        CHECK(stats.mode() == 2.0f);
        CHECK(stats.minority() == 1.0f);
        CHECK(stats.variance() == 0.25f);
        CHECK(stats.stdev() == 0.5f);
        CHECK(stats.weighted_variance() == stats.variance());
        CHECK(stats.weighted_stdev() == stats.stdev());
        CHECK(stats.coefficient_of_variation() == Approx(0.333333f));
        CHECK(stats.weighted_count() == stats.count());
        CHECK(stats.weighted_sum() == stats.sum());
        CHECK(stats.weighted_mean() == stats.mean());
    }

    SECTION("No values defined, all weights defined")
    {
        // Example: population-weighted temperature in dataset covered by pop but without temperature data
        RasterStatsOptions opts;
        opts.store_histogram = true;
        RasterStats<TestType> stats{ opts };
        stats.process(areas, all_values_missing, all_values_defined);

        CHECK(stats.count() == 0);
        CHECK(stats.sum() == 0);
        CHECK(!stats.min().has_value());
        CHECK(!stats.max().has_value());
        CHECK(std::isnan(stats.mean()));
        CHECK(std::isnan(stats.variance()));
        CHECK(std::isnan(stats.stdev()));
        CHECK(std::isnan(stats.weighted_variance()));
        CHECK(std::isnan(stats.weighted_stdev()));
        CHECK(std::isnan(stats.coefficient_of_variation()));
        CHECK(stats.weighted_count() == stats.count());
        CHECK(stats.weighted_sum() == stats.sum());
        CHECK(std::isnan(stats.weighted_mean()));
    }

    SECTION("No values defined, no weights defined")
    {
        RasterStatsOptions opts;
        opts.store_histogram = true;
        RasterStats<TestType> stats{ opts };
        stats.process(areas, all_values_missing, all_values_missing);

        CHECK(stats.count() == 0);
        CHECK(stats.sum() == 0);
        CHECK(!stats.min().has_value());
        CHECK(!stats.max().has_value());
        CHECK(std::isnan(stats.mean()));
        CHECK(std::isnan(stats.variance()));
        CHECK(std::isnan(stats.stdev()));
        CHECK(std::isnan(stats.weighted_variance()));
        CHECK(std::isnan(stats.weighted_stdev()));
        CHECK(std::isnan(stats.coefficient_of_variation()));
        CHECK(stats.weighted_count() == 0);
        CHECK(stats.weighted_sum() == 0);
        CHECK(std::isnan(stats.weighted_mean()));
    }

    SECTION("All values defined, no weights defined")
    {
        // Example: population-weighted temperature in polygon covered by temperature but without pop data
        RasterStatsOptions opts;
        opts.store_histogram = true;
        opts.calc_variance = true;
        RasterStats<TestType> stats{ opts };
        stats.process(areas, all_values_defined, all_values_missing);

        CHECK(stats.count() == 1.0f);
        CHECK(stats.sum() == 2.5f);
        CHECK(stats.min() == 1.0f);
        CHECK(stats.max() == 4.0f);
        CHECK(stats.mean() == 2.5f);
        CHECK(stats.variance() == 1.25f);
        CHECK(stats.stdev() == 1.118034f);
        CHECK(stats.coefficient_of_variation() == 0.4472136f);
        CHECK(std::isnan(stats.weighted_count()));
        CHECK(std::isnan(stats.weighted_sum()));
        CHECK(std::isnan(stats.weighted_mean()));
        CHECK(std::isnan(stats.weighted_variance()));
        CHECK(std::isnan(stats.weighted_stdev()));
    }

    SECTION("All values defined, some weights defined")
    {
        RasterStatsOptions opts;
        opts.store_histogram = true;
        opts.calc_variance = true;
        RasterStats<TestType> stats{ opts };
        stats.process(areas, all_values_defined, some_values_defined);

        CHECK(stats.count() == 1.0f);
        CHECK(stats.sum() == 2.5f);
        CHECK(stats.min() == 1.0f);
        CHECK(stats.max() == 4.0f);
        CHECK(stats.mean() == 2.5f);
        CHECK(stats.variance() == 1.25f);
        CHECK(stats.stdev() == 1.118034f);
        CHECK(stats.coefficient_of_variation() == 0.4472136f);
        CHECK(std::isnan(stats.weighted_count()));
        CHECK(std::isnan(stats.weighted_sum()));
        CHECK(std::isnan(stats.weighted_mean()));
        CHECK(std::isnan(stats.weighted_variance()));
        CHECK(std::isnan(stats.weighted_stdev()));
    }
}

TEST_CASE("Stat subsets calculated for a specific category")
{
    std::vector<int> landcov = { 1, 1, 1, 2, 2, 2 };
    std::vector<float> coverage = { 0.5, 0.4, 0, 0.3, 0.3, 0.2 };
    std::vector<double> weight = { 0.3, 0.4, 1, 4.0, 3.0, 0 };

    RasterStatsOptions opts;
    opts.store_histogram = true;
    RasterStats<decltype(landcov)::value_type> stats{ opts };

    for (std::size_t i = 0; i < landcov.size(); i++) {
        stats.process_value(landcov[i], coverage[i], weight[i]);
    }

    CHECK(stats.count(1).value() == Approx(0.5 + 0.4));
    CHECK(stats.count(2).value() == Approx(0.3 + 0.3 + 0.2));
    CHECK(!stats.count(3).has_value());

    CHECK(stats.frac(1).value() == Approx(stats.count(1).value() / stats.count()));
    CHECK(stats.frac(2).value() == Approx(stats.count(2).value() / stats.count()));
    CHECK(!stats.frac(3).has_value());

    CHECK(stats.weighted_count(1).value() == Approx(0.5 * 0.3 + 0.4 * 0.4));
    CHECK(stats.weighted_count(2).value() == Approx(0.3 * 4.0 + 0.3 * 3.0));
    CHECK(!stats.weighted_count(3).has_value());

    CHECK(stats.weighted_frac(1).value() == Approx(stats.weighted_count(1).value() / stats.weighted_count()));
    CHECK(stats.weighted_frac(2).value() == Approx(stats.weighted_count(2).value() / stats.weighted_count()));
    CHECK(!stats.weighted_frac(3).has_value());
}

TEST_CASE("Iterator provides access to seen values")
{
    std::vector<int> values = { 1, 3, 2 };
    std::vector<float> cov = { 1.0, 2.0, 0.0 };

    RasterStatsOptions opts;
    opts.store_histogram = true;
    RasterStats<decltype(values)::value_type> stats{ opts };

    for (std::size_t i = 0; i < values.size(); i++) {
        stats.process_value(values[i], cov[i], 1.0);
    }

    std::vector<int> found(stats.begin(), stats.end());
    std::sort(found.begin(), found.end());

    CHECK(found.size() == 3);
    CHECK(found[0] == 1);
    CHECK(found[1] == 2);
    CHECK(found[2] == 3);
}

TEST_CASE("Unweighted stats consider all values when part of polygon is inside value raster but outside weighting raster")
{
    GEOSContextHandle_t context = init_geos();

    RasterStatsOptions opts;
    opts.store_histogram = true;
    RasterStats<double> weighted_stats{ opts };
    RasterStats<double> unweighted_stats{ opts };

    Grid<bounded_extent> values_grid{ { 0, 0, 5, 5 }, 1, 1 };  // 5x5 grid
    Grid<bounded_extent> weights_grid{ { 0, 2, 5, 5 }, 1, 1 }; // 3x3 grid

    auto g = GEOSGeom_read_r(context, "POLYGON ((0.5 0.5, 3.5 0.5, 3.5 3.5, 0.5 3.5, 0.5 0.5))");

    auto common_grid = values_grid.common_grid(weights_grid);

    Raster<float> areas = raster_cell_intersection(common_grid, context, g.get());
    Raster<double> values(values_grid);
    Raster<double> weights(weights_grid);
    fill_by_row(values, 1.0, 1.0);
    fill_by_row(weights, 0.1, 0.05);

    weighted_stats.process(areas, values, weights);
    unweighted_stats.process(areas, values);

    CHECK(weighted_stats.count() == unweighted_stats.count());
    CHECK(weighted_stats.max() == unweighted_stats.max());
    CHECK(weighted_stats.mean() == unweighted_stats.mean());
    CHECK(weighted_stats.min() == unweighted_stats.min());
    CHECK(weighted_stats.sum() == unweighted_stats.sum());
}

TEST_CASE("Variance calculations are correct for equally-weighted observations")
{
    std::vector<double> values{ 3.4, 2.9, 1.7, 8.8, -12.7, 100.4, 8.4, 11.3 };

    WestVariance wv;
    for (const auto& x : values) {
        wv.process(x, 3.0);
    }

    CHECK(wv.stdev() == Approx(32.80967)); //
    CHECK(wv.variance() == Approx(1076.474));
    CHECK(wv.coefficent_of_variation() == Approx(2.113344));
}

TEST_CASE("Variance calculations are correct for unequally-weighted observations")
{
    std::vector<double> values{ 3.4, 2.9, 1.7, 8.8, -12.7, 100.4, 8.4, 11.3, 50 };
    std::vector<double> weights{ 1.0, 0.1, 1.0, 0.2, 0.44, 0.3, 0.3, 0.83, 0 };

    WestVariance wv;
    for (size_t i = 0; i < values.size(); i++) {
        wv.process(values[i], weights[i]);
    }

    CHECK(wv.stdev() == Approx(25.90092));                   // output from Weighted.Desc.Stat::w.sd in R
    CHECK(wv.variance() == Approx(670.8578));                // output from Weighted.Desc.Stat::w.var in R
    CHECK(wv.coefficent_of_variation() == Approx(2.478301)); // output from Weighted.Desc.Stat::w.sd / Weighted.Desc.Stat::w.mean
}

TEST_CASE("Variance calculations are correct for unequally-weighted observations, initial zeros")
{
    std::vector<double> values{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    std::vector<double> weights{ 0, 0, 0, 0, 0, 0, 0.25, 0.5, 0.25 };

    WestVariance wv;
    for (size_t i = 0; i < values.size(); i++) {
        wv.process(values[i], weights[i]);
    }

    CHECK(wv.stdev() == Approx(0.7071068));                       // output from Weighted.Desc.Stat::w.sd in R
    CHECK(wv.variance() == Approx(0.5));                          // output from Weighted.Desc.Stat::w.var in R
    CHECK(wv.coefficent_of_variation() == Approx(0.7071068 / 8)); // output from Weighted.Desc.Stat::w.sd / Weighted.Desc.Stat::w.mean
}

TEST_CASE("Weighted quantile calculations are correct for equally-weighted inputs")
{
    std::vector<double> values{ 3.4, 2.9, 1.7, 8.8, -12.7, 100.4, 8.4, 11.3 };

    WeightedQuantiles wq;
    for (const auto& x : values) {
        wq.process(x, 1.7);
    }

    // check values against output of stats::quantile in R
    CHECK(wq.quantile(0) == -12.7);
    CHECK(wq.quantile(0.25) == Approx(2.6));
    CHECK(wq.quantile(0.50) == Approx(5.9));
    CHECK(wq.quantile(0.75) == Approx(9.425));
    CHECK(wq.quantile(1.0) == Approx(100.4));
}

TEST_CASE("Weighted quantile calculations are correct for unequally-weighted inputs")
{
    std::vector<double> values{ 3.4, 2.9, 1.7, 8.8, -12.7, 100.4, 8.4, 11.3, 50 };
    std::vector<double> weights{ 1.0, 0.1, 1.0, 0.2, 0.44, 0.3, 0.3, 0.83, 0 };

    WeightedQuantiles wq;
    for (auto i = 0; i < values.size(); i++) {
        wq.process(values[i], weights[i]);
    }

    // check values against output of wsim.distributions::stack_weighted_quantile in R
    // https://gitlab.com/isciences/wsim/wsim
    CHECK(wq.quantile(0.00) == Approx(-12.7));
    CHECK(wq.quantile(0.25) == Approx(2.336667));
    CHECK(wq.quantile(0.50) == Approx(4.496774));
    CHECK(wq.quantile(0.75) == Approx(9.382437));
    CHECK(wq.quantile(1.00) == Approx(100.4));
}

TEST_CASE("Weighted quantile errors on invalid weights")
{
    CHECK_THROWS(WeightedQuantiles().process(5, -1));
    CHECK_THROWS(WeightedQuantiles().process(3, std::numeric_limits<double>::quiet_NaN()));
    CHECK_THROWS(WeightedQuantiles().process(3, std::numeric_limits<double>::infinity()));
}

TEST_CASE("Weighted quantile errors on invalid quantiles")
{
    WeightedQuantiles wq;
    for (auto i = 0; i < 10; i++) {
        wq.process(i, 1);
    }

    CHECK_THROWS(wq.quantile(1.1));
    CHECK_THROWS(wq.quantile(-0.1));
    CHECK_THROWS(wq.quantile(std::numeric_limits<double>::quiet_NaN()));
}

TEST_CASE("Weighted quantiles are appropriately refreshed")
{
    WeightedQuantiles wq;
    wq.process(1, 1);
    wq.process(2, 1),
      wq.process(3, 1);

    CHECK(wq.quantile(0.5) == 2);

    wq.process(4, 1);

    CHECK(wq.quantile(0.5) == 2.5);
}

}
