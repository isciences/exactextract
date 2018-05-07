#include "catch.hpp"

#include "traversal_areas.h"

TEST_CASE("Exit from same side as entry", "[traversal-areas]" ) {
    Box b{0, 0, 10, 10};

    std::vector<Coordinate> traversal { {7, 0}, {7, 1}, {6, 1}, {6, 0} };
    std::vector<const decltype(traversal)*> traversals { &traversal };

    CHECK( left_hand_area(b, traversals) == 1 );
}


