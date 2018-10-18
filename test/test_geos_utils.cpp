#include "catch.hpp"

#include "geos_utils.h"

using namespace exactextract;

TEST_CASE("Box-polygon conversion", "[geos]") {
    GEOSContextHandle_t context = initGEOS_r(nullptr, nullptr);

    Box b { 3, 2, 7, 9 };

    auto actual = geos_make_box_polygon(context, b);
    auto expected = GEOSGeom_read_r(context, "POLYGON ((3 2, 7 2, 7 9, 3 9, 3 2))");

    CHECK( GEOSEqualsExact_r(context, actual.get(), expected.get(), 0.0) );
    CHECK( b == geos_get_box(context, expected.get()) );

    finishGEOS_r(context);
}

TEST_CASE("Orientation testing", "[geos]") {
    GEOSContextHandle_t context = initGEOS_r(nullptr, nullptr);

    auto ccw = GEOSGeom_read_r(context, "LINEARRING (0 0, 1 0, 1 1, 0 1, 0 0)");
    auto cw = GEOSGeom_read_r(context, "LINEARRING (0 0, 0 1, 1 1, 1 0, 0 0)");
    auto too_short = GEOSGeom_read_r(context, "LINESTRING (0 0, 1 1, 1 3)");

    CHECK( geos_is_ccw(context, GEOSGeom_getCoordSeq_r(context, ccw.get())) );
    CHECK( !geos_is_ccw(context, GEOSGeom_getCoordSeq_r(context, cw.get())) );
    CHECK_THROWS( geos_is_ccw(context, GEOSGeom_getCoordSeq_r(context, too_short.get())) );

    finishGEOS_r(context);
}

TEST_CASE("Segment intersection", "[geos]") {
    GEOSContextHandle_t context = initGEOS_r(nullptr, nullptr);

    Coordinate result;
    bool intersected;
    intersected = segment_intersection(context, {0, 0}, {1, 2}, {0, 1}, {2, 1}, result);

    CHECK( intersected );
    CHECK( result == Coordinate{0.5, 1.0} );

    intersected = segment_intersection(context, {0, 0}, {1, 1}, {2, 2}, {3, 3}, result);
    CHECK( !intersected );

    finishGEOS_r(context);
}

TEST_CASE("Segment orientation", "[geos]") {
    GEOSContextHandle_t context = initGEOS_r(nullptr, nullptr);

    auto right = GEOSGeom_read_r(context, "LINESTRING (0 0, 1 0, 1 1)");
    auto left = GEOSGeom_read_r(context, "LINESTRING (1 0, 0 0, 1 1)");
    auto up = GEOSGeom_read_r(context, "LINESTRING (0 0, 0 1, 1 1)");
    auto down = GEOSGeom_read_r(context, "LINESTRING (0 0, 0 -1, 1 1)");
    auto repeated = GEOSGeom_read_r(context, "LINESTRING (0 0, 0 0, 0 1, 1 1)");
    auto angled = GEOSGeom_read_r(context, "LINESTRING (0 0, 1 1, 1 2)");

    CHECK( initial_segment_orientation(context, GEOSGeom_getCoordSeq_r(context, right.get()))
           == SegmentOrientation::HORIZONTAL_RIGHT );

    CHECK( initial_segment_orientation(context, GEOSGeom_getCoordSeq_r(context, left.get()))
           == SegmentOrientation::HORIZONTAL_LEFT );

    CHECK( initial_segment_orientation(context, GEOSGeom_getCoordSeq_r(context, up.get()))
           == SegmentOrientation::VERTICAL_UP );

    CHECK( initial_segment_orientation(context, GEOSGeom_getCoordSeq_r(context, down.get()))
           == SegmentOrientation::VERTICAL_DOWN );

    CHECK( initial_segment_orientation(context, GEOSGeom_getCoordSeq_r(context, repeated.get()))
           == SegmentOrientation::VERTICAL_UP );

    CHECK( initial_segment_orientation(context, GEOSGeom_getCoordSeq_r(context, angled.get()))
           == SegmentOrientation::ANGLED );

    finishGEOS_r(context);
}
