#include "catch.hpp"

#include "box.h"

TEST_CASE("Box dimensions are calculated correctly", "[box]" ) {
    Box b{5, 7, 7, 10};

    CHECK( b.width() == 2 );
    CHECK( b.height() == 3 );
    CHECK( b.area() == 6 );
}

TEST_CASE("Coordinate sides are correctly identified", "[box]") {
    Box b{0, 0, 13, 17};

    CHECK(Side::LEFT == b.side({0, 5}));
    CHECK(Side::RIGHT == b.side({13, 5}));
    CHECK(Side::TOP == b.side({5, 17}));
    CHECK(Side::BOTTOM == b.side({5, 0}));
    CHECK(Side::NONE == b.side({5, 5}));
}

TEST_CASE("Crossing is correctly determined", "[box]") {
    Box b{0, 0, 1, 1};
    double tol = 1e-14;

    // vertical up
    Crossing crx = b.crossing({0.4, 0.6}, {0.4, 11});
    CHECK( crx.side() == Side::TOP );
    CHECK( crx.coord().equals({0.4, 1}, tol) );

    // vertical down
    crx = b.crossing({0.4, 0.6}, {0.4, -11});
    CHECK( crx.side() == Side::BOTTOM );
    CHECK( crx.coord().equals({0.4, 0}, tol) );

    // horizontal left
    crx = b.crossing({0.4, 0.6}, {-11, 0.6});
    CHECK( crx.side() == Side::LEFT );
    CHECK( crx.coord().equals({0, 0.6}, tol) );

    // horizontal right
    crx = b.crossing({0.4, 0.6}, {11, 0.6});
    CHECK( crx.side() == Side::RIGHT );
    CHECK( crx.coord().equals({1, 0.6}, tol) );

    // upper-right (1)
    crx = b.crossing({0.4, 0.6}, {1.2, 0.8});
    CHECK( crx.side() == Side::RIGHT );
    CHECK( crx.coord().equals({1, 0.75}, tol) );

    // upper-right (2)
    crx = b.crossing({0.4, 0.6}, {0.7, 1.2});
    CHECK( crx.side() == Side::TOP );
    CHECK( crx.coord().equals({0.6, 1}, tol) );

    // upper-left (1)
    crx = b.crossing({0.4, 0.6}, {0.1, 1.2});
    CHECK( crx.side() == Side::TOP );
    CHECK( crx.coord().equals({0.2, 1}, tol) );

    // upper-left (2)
    crx = b.crossing({0.4, 0.6}, {-0.4, 1.2});
    CHECK( crx.side() == Side::LEFT );
    CHECK( crx.coord().equals({0, 0.9}, tol) );

    // upper-left (2)
    crx = b.crossing({0.4, 0.6}, {-0.4, 1.2});
    CHECK( crx.side() == Side::LEFT );
    CHECK( crx.coord().equals({0, 0.9}, tol) );

    // lower-left (1)
    crx = b.crossing({0.4, 0.6}, {-0.4, 0.2});
    CHECK( crx.side() == Side::LEFT );
    CHECK( crx.coord().equals({0, 0.4}, tol) );

    // lower-left (2)
    crx = b.crossing({0.4, 0.6}, {0.1, -0.3});
    CHECK( crx.side() == Side::BOTTOM );
    CHECK( crx.coord().equals({0.2, 0}, tol) );

    // lower-right (1)
    crx = b.crossing({0.4, 0.6}, {0.7, -0.3});
    CHECK( crx.side() == Side::BOTTOM );
    CHECK( crx.coord().equals({0.6, 0}, tol) );

    // lower-right (2)
    crx = b.crossing({0.4, 0.6}, {1.6, 0});
    CHECK( crx.side() == Side::RIGHT );
    CHECK( crx.coord().equals({1, 0.3}, tol) );
}


