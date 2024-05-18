#include <catch.hpp>

#include "map_feature.h"

using exactextract::MapFeature;

TEMPLATE_TEST_CASE("32-bit int fields", "[feature]", std::int8_t, std::int16_t, std::int32_t)
{
    auto x = std::numeric_limits<TestType>::max();

    MapFeature mf;
    mf.set("name", x);

    CHECK(mf.field_type("name") == MapFeature::ValueType::INT);
    CHECK(mf.get_int("name") == x);
}

TEMPLATE_TEST_CASE("64-bit int fields", "[feature]", std::int64_t, std::size_t)
{
    auto x = std::numeric_limits<std::int64_t>::max();

    MapFeature mf;
    mf.set("name", x);

    CHECK(mf.field_type("name") == MapFeature::ValueType::INT64);
    CHECK(mf.get_int64("name") == x);
}

TEST_CASE("exception thrown on oversize unsigned int", "[feature]")
{
    auto x = static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()) + 1;

    MapFeature mf;
    CHECK_THROWS_WITH(mf.set("field", static_cast<uint64_t>(x)), Catch::StartsWith("Value is too large"));
}

TEST_CASE("exception thrown on oversized value in unsigned int array", "[feature]")
{
    auto big = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1;

    std::vector<std::uint64_t> vec{ 1, 2, big, 4 };

    MapFeature mf;
    CHECK_THROWS_WITH(mf.set("field", vec), Catch::StartsWith("Array value too large"));
}

TEMPLATE_TEST_CASE("floating point fields", "[feature]", float, double)
{
    TestType x = 123.456;

    MapFeature mf;
    mf.set("name", x);

    CHECK(mf.get_double("name") == x);
    CHECK(mf.field_type("name") == MapFeature::ValueType::DOUBLE);
}

TEST_CASE("string fields", "[feature]")
{
    MapFeature mf;
    mf.set("name", "abcdef");

    CHECK(mf.field_type("name") == MapFeature::ValueType::STRING);
    CHECK(mf.get_string("name") == "abcdef");
}

TEMPLATE_TEST_CASE("int32 array fields", "[feature]", std::vector<std::int8_t>, std::vector<std::int16_t>, std::vector<std::int32_t>)
{
    TestType x{ 4, 5, 7 };

    MapFeature mf;
    mf.set("name", x);

    CHECK(mf.field_type("name") == MapFeature::ValueType::INT_ARRAY);
    CHECK(mf.get_integer_array("name").size == 3);
    CHECK(mf.get_integer_array("name").data[2] == 7);
}

TEMPLATE_TEST_CASE("int64 array fields", "[feature]", std::vector<std::int64_t>)
{
    auto y = std::numeric_limits<std::int64_t>::max() - 2;
    TestType x{ y, y + 1, y + 2 };

    MapFeature mf;
    mf.set("name", x);

    CHECK(mf.field_type("name") == MapFeature::ValueType::INT64_ARRAY);
    CHECK(mf.get_integer64_array("name").size == 3);
    CHECK(mf.get_integer64_array("name").data[2] == y + 2);
}

TEMPLATE_TEST_CASE("float array fields", "[feature]", std::vector<float>, std::vector<double>)
{
    TestType x{ 4.4, 5.5, 7.7 };

    MapFeature mf;
    mf.set("name", x);

    CHECK(mf.field_type("name") == MapFeature::ValueType::DOUBLE_ARRAY);
    CHECK(mf.get_double_array("name").size == 3);
    CHECK(mf.get_double_array("name").data[2] == static_cast<typename TestType::value_type>(7.7));
}
