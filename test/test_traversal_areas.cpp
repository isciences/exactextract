#include "catch.hpp"

#include "traversal_areas.h"

using namespace exactextract;

using TraversalVector = std::vector<const std::vector<Coordinate>*>;

template<typename T>
bool
geometry_equals(GEOSContextHandle_t context, const T& geom, std::string expected)
{
    auto expected_geom = geos_ptr(context, GEOSGeomFromWKT_r(context, expected.c_str()));

    CHECK(expected_geom != nullptr);

    return GEOSEquals_r(context, &*geom, expected_geom.get());
}

TEST_CASE("Exit from same side as entry", "[traversal-areas]")
{
    GEOSContextHandle_t context = GEOS_init_r();
    Box b{ 0, 0, 10, 10 };

    std::vector<Coordinate> traversal{ { 7, 0 }, { 7, 1 }, { 6, 1 }, { 6, 0 } };
    TraversalVector traversals{ &traversal };

    CHECK(left_hand_area(b, traversals) == 1);
    CHECK(geometry_equals(context, left_hand_rings(context, b, traversals), "POLYGON ((6 0, 7 0, 7 1, 6 1, 6 0))"));

    std::reverse(traversal.begin(), traversal.end());
    CHECK(left_hand_area(b, traversals) == 99);
    CHECK(geometry_equals(context, left_hand_rings(context, b, traversals), "POLYGON ((0 0, 6 0, 6 1, 7 1, 7 0, 10 0, 10 10, 0 10, 0 0))"));

    GEOS_finish_r(context);
}

TEST_CASE("Enter bottom, exit left", "[traversal-areas]")
{
    GEOSContextHandle_t context = GEOS_init_r();
    Box b{ 0, 0, 10, 10 };

    std::vector<Coordinate> traversal{ { 5, 0 }, { 5, 5 }, { 0, 5 } };
    TraversalVector traversals{ &traversal };

    CHECK(left_hand_area(b, traversals) == 25);
    CHECK(geometry_equals(context, left_hand_rings(context, b, traversals), "POLYGON ((0 0, 5 0, 5 5, 0 5, 0 0))"));

    GEOS_finish_r(context);
}

TEST_CASE("Enter bottom, exit top", "[traversal-areas]")
{
    GEOSContextHandle_t context = GEOS_init_r();
    Box b{ 0, 0, 10, 10 };

    std::vector<Coordinate> traversal{ { 4, 0 }, { 4, 10 } };
    TraversalVector traversals{ &traversal };

    CHECK(left_hand_area(b, traversals) == 40);
    CHECK(geometry_equals(context, left_hand_rings(context, b, traversals), "POLYGON ((0 0, 4 0, 4 10, 0 10, 0 0))"));

    GEOS_finish_r(context);
}

TEST_CASE("Multiple traversals (basic)", "[traversal-areas]")
{
    GEOSContextHandle_t context = GEOS_init_r();
    Box b{ 0, 0, 10, 10 };

    std::vector<Coordinate> t1 = { { 2, 10 }, { 2, 0 } };
    std::vector<Coordinate> t2 = { { 4, 0 }, { 4, 10 } };

    TraversalVector traversals{ &t1, &t2 };

    CHECK(left_hand_area(b, traversals) == 20);
    CHECK(geometry_equals(context, left_hand_rings(context, b, traversals), "POLYGON ((2 0, 4 0, 4 10, 2 10, 2 0))"));

    GEOS_finish_r(context);
}

TEST_CASE("Multiple traversals", "[traversal-areas]")
{
    GEOSContextHandle_t context = GEOS_init_r();
    Box b{ 0, 0, 10, 10 };

    std::vector<Coordinate> t1 = { { 2, 0 }, { 2, 2 }, { 0, 2 } }; // 2x2 = 4
    std::vector<Coordinate> t2 = { { 3, 10 }, { 3, 0 } };
    std::vector<Coordinate> t3 = { { 5, 0 }, { 5, 10 } };                      // 2x10 = 20
    std::vector<Coordinate> t4 = { { 8, 10 }, { 10, 8 } };                     // 2x2/2 = 2
    std::vector<Coordinate> t5 = { { 10, 6 }, { 8, 6 }, { 8, 3 }, { 10, 3 } }; // 2x3 = 6
    std::vector<Coordinate> t6 = { { 10, 4 }, { 9, 4 }, { 9, 5 }, { 10, 5 } }; // 1x1 = 1 (subtracted)
    std::vector<Coordinate> t7 = { { 10, 3 }, { 8, 3 }, { 8, 0 } };            // 2x3 = 6

    TraversalVector traversals{ &t1, &t2, &t3, &t4, &t5, &t6, &t7 };

    CHECK(left_hand_area(b, traversals) == 4 + 20 + 2 + 6 - 1 + 6);
    CHECK(geometry_equals(context, left_hand_rings(context, b, traversals), "MULTIPOLYGON (((2 0, 2 2, 0 2, 0 0, 2 0)), "
                                                                            "((3 10, 3 0, 5 0, 5 10, 3 10)), "
                                                                            "((8 10, 10 8, 10 10, 8 10)), "
                                                                            "((10 6, 8 6, 8 3, 10 3, 10 3, 8 3, 8 0, 10 0, 10 4, 9 4, 9 5, 10 5, 10 6)))"));

    GEOS_finish_r(context);
}

TEST_CASE("No traversals", "[traversal-areas]")
{
    GEOSContextHandle_t context = GEOS_init_r();
    Box b{ 0, 0, 10, 10 };

    TraversalVector traversals;

    CHECK_THROWS(left_hand_area(b, traversals));
    CHECK_THROWS(left_hand_rings(context, b, traversals));

    GEOS_finish_r(context);
}

TEST_CASE("Point traversal", "[traversal-areas]")
{
    GEOSContextHandle_t context = GEOS_init_r();
    Box b{ 0, 0, 10, 10 };

    std::vector<Coordinate> t1{ { 4, 0 }, { 4, 0 } };
    TraversalVector traversals{ &t1 };

    CHECK_THROWS(left_hand_area(b, traversals) == 100);
    CHECK_THROWS(left_hand_rings(context, b, traversals));

    GEOS_finish_r(context);
}

TEST_CASE("Closed ring ccw", "[traversal-areas]")
{
    GEOSContextHandle_t context = GEOS_init_r();
    Box b{ 0, 0, 10, 10 };

    std::vector<Coordinate> t1 = { { 1, 1 }, { 2, 1 }, { 2, 2 }, { 1, 2 }, { 1, 1 } };
    TraversalVector traversals{ &t1 };

    CHECK(left_hand_area(b, traversals) == 1);
    CHECK(geometry_equals(context, left_hand_rings(context, b, traversals), "POLYGON ((1 1, 2 1, 2 2, 1 2, 1 1))"));

    GEOS_finish_r(context);
}

TEST_CASE("Closed ring ccw overlapping edge", "[traversal-areas]")
{
    GEOSContextHandle_t context = GEOS_init_r();
    Box b{ 0, 0, 10, 10 };

    std::vector<Coordinate> t1 = { { 1, 0 }, { 2, 1 }, { 1, 1 }, { 1, 0 } };
    TraversalVector traversals{ &t1 };

    CHECK(left_hand_area(b, traversals) == 0.5);
    CHECK(geometry_equals(context, left_hand_rings(context, b, traversals), "POLYGON ((1 0, 1 1, 2 1, 1 0))"));

    GEOS_finish_r(context);
}

TEST_CASE("Closed ring cw", "[traversal-areas]")
{
    GEOSContextHandle_t context = GEOS_init_r();
    Box b{ 0, 0, 10, 10 };

    std::vector<Coordinate> t1 = { { 1, 1 }, { 1, 2 }, { 2, 2 }, { 2, 1 }, { 1, 1 } };
    TraversalVector traversals{ &t1 };

    CHECK(left_hand_area(b, traversals) == 99);
    CHECK(geometry_equals(context, left_hand_rings(context, b, traversals), "POLYGON ((0 0, 10 0, 10 10, 0 10, 0 0), (1 1, 1 2, 2 2, 2 1, 1 1))"));

    GEOS_finish_r(context);
}

TEST_CASE("Closed ring cw with point traversal", "[traversal-areas]")
{
    GEOSContextHandle_t context = GEOS_init_r();
    Box b{ 0, 0, 10, 10 };

    std::vector<Coordinate> t1 = { { 1, 1 }, { 1, 2 }, { 2, 2 }, { 2, 1 }, { 1, 1 } };
    std::vector<Coordinate> t2 = { { 10, 5 }, { 10, 5 } };
    TraversalVector traversals{ &t1, &t2 };

    CHECK(left_hand_area(b, traversals) == 99);
    CHECK(geometry_equals(context, left_hand_rings(context, b, traversals), "POLYGON ((0 0, 10 0, 10 10, 0 10, 0 0), (1 1, 1 2, 2 2, 2 1, 1 1))"));

    GEOS_finish_r(context);
}

TEST_CASE("Closed ring cw touching edge at node", "[traversal-areas]")
{
    GEOSContextHandle_t context = GEOS_init_r();
    Box b{ 0, 0, 10, 10 };

    std::vector<Coordinate> t1 = { { 0, 0 }, { 2, 2 }, { 3, 2 }, { 0, 0 } };
    TraversalVector traversals{ &t1 };

    CHECK(left_hand_area(b, traversals) == 99);
    CHECK(geometry_equals(context, left_hand_rings(context, b, traversals), "POLYGON ((0 0, 10 0, 10 10, 0 10, 0 0), (0 0, 2 2, 3 2, 0 0))"));

    GEOS_finish_r(context);
}

TEST_CASE("Closed ring cw touching edge interior", "[traversal-areas]")
{
    GEOSContextHandle_t context = GEOS_init_r();
    Box b{ 0, 0, 10, 10 };

    std::vector<Coordinate> t1 = { { 1, 0 }, { 2, 2 }, { 3, 2 }, { 1, 0 } };
    TraversalVector traversals{ &t1 };

    CHECK(left_hand_area(b, traversals) == 99);
    CHECK(geometry_equals(context, left_hand_rings(context, b, traversals), "POLYGON ((0 0, 10 0, 10 10, 0 10, 0 0), (1 0, 2 2, 3 2, 1 0))"));

    GEOS_finish_r(context);
}

TEST_CASE("Closed ring cw overlapping edge", "[traversal-areas]")
{
    GEOSContextHandle_t context = GEOS_init_r();
    Box b{ 0, 0, 10, 10 };

    std::vector<Coordinate> t1 = { { 1, 0 }, { 1, 1 }, { 2, 1 }, { 1, 0 } };
    TraversalVector traversals{ &t1 };

    CHECK(left_hand_area(b, traversals) == 99.5);
    CHECK(geometry_equals(context, left_hand_rings(context, b, traversals), "POLYGON ((0 0, 1 0, 1 1, 2 1, 1 0, 10 0, 10 10, 0 10, 0 0))"));

    GEOS_finish_r(context);
}

TEST_CASE("Edge traversal (interior left)", "[traversal-areas]")
{
    GEOSContextHandle_t context = GEOS_init_r();
    Box b{ 0, 0, 10, 10 };

    std::vector<Coordinate> t1{ { 4, 0 }, { 10, 0 } };
    TraversalVector traversals{ &t1 };

    CHECK(left_hand_area(b, traversals) == 100);
    CHECK(geometry_equals(context, left_hand_rings(context, b, traversals), "POLYGON ((0 0, 4 0, 10 0, 10 10, 0 10, 0 0))"));

    GEOS_finish_r(context);
}

TEST_CASE("Edge traversal (interior right)", "[traversal-areas]")
{
    GEOSContextHandle_t context = GEOS_init_r();
    Box b{ 2, 2, 3, 3 };

    std::vector<Coordinate> t1{ { 2, 2 }, { 2, 2.5 }, { 2, 2.5 } };
    TraversalVector traversals{ &t1 };

    CHECK(left_hand_area(b, traversals) == 0);
    CHECK(geometry_equals(context, left_hand_rings(context, b, traversals), "POLYGON EMPTY"));

    GEOS_finish_r(context);
}
