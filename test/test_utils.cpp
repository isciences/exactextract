#include "catch.hpp"

#include "utils.h"

using exactextract::parse_dataset_descriptor;
using exactextract::parse_raster_descriptor;
using exactextract::parse_stat_descriptor;

TEST_CASE("Parsing feature descriptor: no layer specified")
{
    auto descriptor = "countries.shp";

    auto parsed = parse_dataset_descriptor(descriptor);

    CHECK(parsed.first == "countries.shp");
    CHECK(parsed.second == "0");
}

TEST_CASE("Parsing feature descriptor with layer")
{
    auto descriptor = "PG:dbname=postgres port=5432[countries]";

    auto parsed = parse_dataset_descriptor(descriptor);

    CHECK(parsed.first == "PG:dbname=postgres port=5432");
    CHECK(parsed.second == "countries");
}

TEST_CASE("Parsing raster descriptor: file with name and band")
{
    auto descriptor = "pop:gpw_v4.tif[27]";

    auto parsed = parse_raster_descriptor(descriptor);

    CHECK(std::get<0>(parsed) == "pop");
    CHECK(std::get<1>(parsed) == "gpw_v4.tif");
    CHECK(std::get<2>(parsed) == 27);
}

TEST_CASE("Parsing raster descriptor: file with no band")
{
    auto descriptor = "land_area:gpw_v4_land.tif";

    auto parsed = parse_raster_descriptor(descriptor);

    CHECK(std::get<0>(parsed) == "land_area");
    CHECK(std::get<1>(parsed) == "gpw_v4_land.tif");
    CHECK(std::get<2>(parsed) == 0);
}

TEST_CASE("Parsing raster descriptor: file with no name and no band")
{
    auto descriptor = "gpw_v4_land.tif";

    auto parsed = parse_raster_descriptor(descriptor);

    CHECK(std::get<0>(parsed) == "");
    CHECK(std::get<1>(parsed) == "gpw_v4_land.tif");
    CHECK(std::get<2>(parsed) == 0);
}

TEST_CASE("Parsing raster descriptor: file with no name but a specific band")
{
    auto descriptor = "gpw_v4_land.tif[8]";

    auto parsed = parse_raster_descriptor(descriptor);

    CHECK(std::get<0>(parsed) == "");
    CHECK(std::get<1>(parsed) == "gpw_v4_land.tif");
    CHECK(std::get<2>(parsed) == 8);
}

TEST_CASE("Parsing ugly raster descriptor")
{
    auto descriptor = "gpw[3]:gpw_v4_land.tif";

    auto parsed = parse_raster_descriptor(descriptor);

    CHECK(std::get<0>(parsed) == "gpw[3]");
    CHECK(std::get<1>(parsed) == "gpw_v4_land.tif");
    CHECK(std::get<2>(parsed) == 0);
}

TEST_CASE("Degenerate raster descriptor")
{
    CHECK_THROWS_WITH(parse_raster_descriptor(""), "Empty descriptor.");
    CHECK_THROWS_WITH(parse_raster_descriptor(":"), "Descriptor has no filename.");
}

TEST_CASE("Parsing stat descriptor with no weighting")
{
    auto descriptor = parse_stat_descriptor("sum(population)");

    CHECK(descriptor.name == "");
    CHECK(descriptor.stat == "sum");
    CHECK(descriptor.values == "population");
    CHECK(descriptor.weights == "");
}

TEST_CASE("Parsing stat descriptor with arguments and no weighting")
{
    auto descriptor = parse_stat_descriptor("sum(population,min_frac=-1)");

    CHECK(descriptor.name == "");
    CHECK(descriptor.stat == "sum");
    CHECK(descriptor.values == "population");
    CHECK(descriptor.weights == "");
    CHECK(descriptor.args["min_frac"] == "-1");
}

TEST_CASE("Parsing stat descriptor with weighting")
{
    auto descriptor = parse_stat_descriptor("mean(deficit,population)");

    CHECK(descriptor.name == "");
    CHECK(descriptor.stat == "mean");
    CHECK(descriptor.values == "deficit");
    CHECK(descriptor.weights == "population");
}

TEST_CASE("Parsing stat descriptor with weighting and arguments")
{
    auto descriptor = parse_stat_descriptor("quantile(deficit,population,q=0.75)");

    CHECK(descriptor.name == "");
    CHECK(descriptor.stat == "quantile");
    CHECK(descriptor.values == "deficit");
    CHECK(descriptor.weights == "population");
    CHECK(descriptor.args["q"] == "0.75");
}

TEST_CASE("Spaces allowed between arguments")
{
    auto descriptor = parse_stat_descriptor("quantile(deficit, population, q=0.75)");

    CHECK(descriptor.name == "");
    CHECK(descriptor.stat == "quantile");
    CHECK(descriptor.values == "deficit");
    CHECK(descriptor.weights == "population");
    CHECK(descriptor.args["q"] == "0.75");
}

TEST_CASE("Parsing stat descriptor with name and weighting")
{
    auto descriptor = parse_stat_descriptor("pop_weighted_mean_deficit=mean(deficit,population)");

    CHECK(descriptor.name == "pop_weighted_mean_deficit");
    CHECK(descriptor.stat == "mean");
    CHECK(descriptor.values == "deficit");
    CHECK(descriptor.weights == "population");
}

TEST_CASE("Parsing stat descriptor with no arguments")
{
    auto descriptor = parse_stat_descriptor("mean");

    CHECK(descriptor.stat == "mean");
    CHECK(descriptor.name == "");
    CHECK(descriptor.values == "");
    CHECK(descriptor.weights == "");
}

TEST_CASE("Parsing stat descriptor with only keyword arguments")
{
    auto descriptor = parse_stat_descriptor("mean(ignore_nodata=false)");

    CHECK(descriptor.stat == "mean");
    CHECK(descriptor.name == "");
    CHECK(descriptor.values == "");
    CHECK(descriptor.weights == "");
    CHECK(descriptor.args["ignore_nodata"] == "false");
}

TEST_CASE("Parsing stat descriptor with name and no arguments")
{
    auto descriptor = parse_stat_descriptor("pop_mean=mean");

    CHECK(descriptor.stat == "mean");
    CHECK(descriptor.name == "pop_mean");
    CHECK(descriptor.values == "");
    CHECK(descriptor.weights == "");
}

TEST_CASE("Parsing degenerate stat descriptors")
{
    CHECK_THROWS_WITH(parse_stat_descriptor(""), Catch::StartsWith("Invalid stat descriptor."));
    CHECK_THROWS_WITH(parse_stat_descriptor("sum(a,b,c)"), Catch::StartsWith("Invalid stat descriptor."));
    CHECK_THROWS_WITH(parse_stat_descriptor("sum banana"), Catch::StartsWith("Invalid stat descriptor"));
    CHECK_THROWS_WITH(parse_stat_descriptor("sum(b=2,a)"), Catch::StartsWith("Invalid stat descriptor."));
    CHECK_THROWS_WITH(parse_stat_descriptor("sum(a,b=2,b=3)"), Catch::StartsWith("Invalid stat descriptor."));
    CHECK_THROWS_WITH(parse_stat_descriptor("sum(,a)"), Catch::StartsWith("Invalid stat descriptor."));
}
