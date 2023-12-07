#include "catch.hpp"

#include "perimeter_distance.h"

using namespace exactextract;

TEST_CASE("Perimeter distance from origin", "[perimeter-distance]")
{
    Box b{ 0, 0, 13, 17 };

    CHECK(perimeter_distance(b, { 0, 0 }) == 0);
    CHECK(perimeter_distance(b, { 0, 5 }) == 5);
    CHECK(perimeter_distance(b, { 0, 17 }) == 17);
    CHECK(perimeter_distance(b, { 6, 17 }) == 23);
    CHECK(perimeter_distance(b, { 13, 17 }) == 30);
    CHECK(perimeter_distance(b, { 13, 14 }) == 33);
    CHECK(perimeter_distance(b, { 13, 0 }) == 47);
    CHECK(perimeter_distance(b, { 5, 0 }) == 55);
}

TEST_CASE("CCW perimeter distance between two measures", "[perimeter-distance]")
{
    CHECK(perimeter_distance_ccw(3, 2, 100) == 1);
    CHECK(perimeter_distance_ccw(0.5, 3.5, 4) == 1);
    CHECK(perimeter_distance_ccw(1, 1, 100) == 0);
}
