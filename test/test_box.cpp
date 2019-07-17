#include "catch.hpp"

#include "box.h"

using namespace exactextract;

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

TEST_CASE("Boxes with empty or negative dimensions are considered empty", "[box]") {
    CHECK( !Box{0, 0, 1, 1}.empty() ); // normal box
    CHECK( Box{0, 0, 0, 1}.empty() ); // no width
    CHECK( Box{0, 0, 1, 0}.empty() ); // no height
    CHECK( Box{0, 0, 0, 0}.empty() ); // no width or height
    CHECK( Box{0, 0, -1, 1}.empty() ); // negative width
    CHECK( Box{0, 0, 1, -1}.empty() ); // negative height
    CHECK( Box{0, 0, -1, -1}.empty() ); // negative width and height
}

TEST_CASE("Box intersection test is correct", "[box]") {
    Box a = {0, 0, 10, 10};
    Box b = {20, 20, 30, 30};
    Box c = {5, 5, 25, 25 };
    Box d = {1, 1, 2, 2};

    CHECK( !a.intersects(b) );
    CHECK( !b.intersects(a) );

    CHECK( a.intersects(c) );
    CHECK( c.intersects(a) );
    CHECK( b.intersects(c) );
    CHECK( c.intersects(b) );

    CHECK( a.intersects(d) );
    CHECK( d.intersects(a) );
    CHECK( !b.intersects(d) );
    CHECK( !d.intersects(b) );
}

TEST_CASE("Box intersection calculations are correct", "[box]") {
    Box a = {0, 1, 9, 10};
    Box b = {21, 22, 27, 29};
    Box c = {5, 4, 25, 24 };
    Box d = {1, 2, 3, 4};

    CHECK( a.intersection(a) == a );
    CHECK( a.intersection(b).empty() );
    CHECK( a.intersection(c) == Box{5, 4, 9, 10} );
    CHECK( a.intersection(d) == d );

    CHECK( b.intersection(a).empty() );
    CHECK( b.intersection(b) == b );
    CHECK( b.intersection(c) == Box{21, 22, 25, 24} );
    CHECK( b.intersection(d).empty() );

    CHECK( c.intersection(a) == a.intersection(c) );
    CHECK( c.intersection(b) == b.intersection(c) );
    CHECK( c.intersection(c) == c );
    CHECK( c.intersection(d).empty() );

    CHECK( d.intersection(a) == d );
    CHECK( d.intersection(b).empty() );
    CHECK( d.intersection(c).empty() );
    CHECK( d.intersection(d) == d );
}

TEST_CASE("Boxes can be expanded to include other boxes", "[box]") {
    Box a = {0, 1, 9, 10};
    Box b = {21, 22, 27, 29};
    Box c = {5, 4, 25, 24 };
    Box d = {1, 2, 3, 4};
    Box e = {0, 0, -1, -1}; // empty

    CHECK( a.expand_to_include(a) == a );
    CHECK( a.expand_to_include(b) == Box{0, 1, 27, 29} );
    CHECK( a.expand_to_include(c) == Box{0, 1, 25, 24} );
    CHECK( a.expand_to_include(d) == a );
    CHECK( a.expand_to_include(e) == a );

    CHECK( b.expand_to_include(a) == a.expand_to_include(b) );
    CHECK( b.expand_to_include(b) == b );
    CHECK( b.expand_to_include(c) == Box{5, 4, 27, 29} );
    CHECK( b.expand_to_include(d) == Box{1, 2, 27, 29} );
    CHECK( b.expand_to_include(e) == b );

    CHECK( c.expand_to_include(a) == a.expand_to_include(c) );
    CHECK( c.expand_to_include(b) == b.expand_to_include(c) );
    CHECK( c.expand_to_include(c) == c );
    CHECK( c.expand_to_include(d) == Box{1, 2, 25, 24} );
    CHECK( c.expand_to_include(e) == c );

    CHECK( d.expand_to_include(a) == a.expand_to_include(d) );
    CHECK( d.expand_to_include(b) == b.expand_to_include(d) );
    CHECK( d.expand_to_include(c) == c.expand_to_include(d) );
    CHECK( d.expand_to_include(d) == d );
    CHECK( d.expand_to_include(e) == d );

    CHECK( e.expand_to_include(a) == a);
    CHECK( e.expand_to_include(b) == b);
    CHECK( e.expand_to_include(c) == c);
    CHECK( e.expand_to_include(d) == d);
    CHECK( e.expand_to_include(e).empty() );
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


