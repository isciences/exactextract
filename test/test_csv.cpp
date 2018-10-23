#include "catch.hpp"

#include "csv_utils.h"
#include "raster_cell_intersection.h"
#include "raster_stats.h"

#include <sstream>

using namespace exactextract;

TEST_CASE("Simple CSV header when weights are not used") {
    std::ostringstream out;

    write_csv_header("id_field", {"mean", "sum"}, 0, out);

    CHECK( out.str() == "id_field,mean,sum\n" );
}

TEST_CASE("CSV header with one set of weights") {
    std::ostringstream out;

    write_csv_header("id_field", {"mean", "sum"}, 1, out);

    CHECK( out.str() == "id_field,mean,sum\n" );
}

TEST_CASE("CSV header with two sets of weights") {
    std::ostringstream out;

    write_csv_header("id_field", {"mean", "sum"}, 2, out);

    CHECK( out.str() == "id_field,mean_1,sum_1,mean_2,sum_2\n" );
}

TEST_CASE("Writing rows of output for one set of stats") {
    std::ostringstream out;

    Box extent{0, 0, 1, 1};

    Raster<float> intersection_percentages{ {{{1.0, 1.0},{1.0, 1.0}}}, extent};
    Raster<double> values{ {{{1.0, 2.0},{3.0, 4.0}}}, extent};

    RasterStats<double> stats;
    stats.process(intersection_percentages, values);

    write_stats_to_csv("35", stats, {"mean", "sum"}, out);

    CHECK( out.str() == "35,2.5,10\n" );
}

TEST_CASE("Writing rows of output for >1 set of stats") {
    std::ostringstream out;

    Box extent{0, 0, 1, 1};

    Raster<float> intersection_percentages{ {{{1.0, 1.0},{1.0, 1.0}}}, extent};
    Raster<double> values{ {{{1.0, 2.0},{3.0, 4.0}}}, extent};

    RasterStats<double> stats1, stats2;
    stats1.process(intersection_percentages, values);

    stats2.process(intersection_percentages, values);
    stats2.process(intersection_percentages, values);

    write_stats_to_csv("35", {stats1, stats2}, {"mean", "sum"}, out);

    CHECK( out.str() == "35,2.5,10,2.5,20\n" );
}
