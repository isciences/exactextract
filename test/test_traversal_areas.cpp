#include "catch.hpp"

#include "traversal_areas.h"

using namespace exactextract;

using TraversalVector = std::vector<const std::vector<Coordinate>*>;

TEST_CASE("Exit from same side as entry", "[traversal-areas]" ) {
    Box b{0, 0, 10, 10};

    std::vector<Coordinate> traversal { {7, 0}, {7, 1}, {6, 1}, {6, 0} };
    TraversalVector traversals { &traversal };

    CHECK( left_hand_area(b, traversals) == 1 );

    std::reverse(traversal.begin(), traversal.end());
    CHECK( left_hand_area(b, traversals) == 99 );
}

TEST_CASE("Enter bottom, exit left", "[traversal-areas]") {
    Box b{0, 0, 10, 10};

    std::vector<Coordinate> traversal { {5, 0}, {5, 5}, {0, 5} };
    TraversalVector traversals { &traversal };

    CHECK( left_hand_area(b, traversals) == 25 );
}

TEST_CASE("Enter bottom, exit top", "[traversal-areas]") {
    Box b{0, 0, 10, 10};

    std::vector<Coordinate> traversal { {4, 0}, {4, 10} };
    TraversalVector traversals { &traversal };

    CHECK( left_hand_area(b, traversals) == 40 );
}

TEST_CASE("Multiple traversals (basic)", "[traversal-areas]") {
    Box b{0, 0, 10, 10};

    std::vector<Coordinate> t1 = { {2, 10}, {2,  0} };
    std::vector<Coordinate> t2 = { {4,  0}, {4, 10} };

    TraversalVector traversals{ &t1, &t2 };

    CHECK( left_hand_area(b, traversals) == 20 );
}

TEST_CASE("Multiple traversals", "[traversal-areas]") {
    Box b{0, 0, 10, 10};

    std::vector<Coordinate> t1 = { { 2,  0}, { 2,  2}, {0, 2} }; // 2x2 = 4
    std::vector<Coordinate> t2 = { { 3, 10}, { 3,  0} };
    std::vector<Coordinate> t3 = { { 5,  0}, { 5, 10} };         // 2x10 = 20
    std::vector<Coordinate> t4 = { { 8, 10}, {10,  8} };         // 2x2/2 = 2
    std::vector<Coordinate> t5 = { {10,  6}, { 8,  6}, {8, 3}, {10, 3} }; // 2x3 = 6
    std::vector<Coordinate> t6 = { {10,  4}, { 9,  4}, {9, 5}, {10, 5} }; // 1x1 = 1 (subtracted)
    std::vector<Coordinate> t7 = { {10,  3}, { 8,  3}, {8, 0} }; // 2x3 = 6

    TraversalVector traversals{ &t1, &t2, &t3, &t4, &t5, &t6, &t7 };

    CHECK( left_hand_area(b, traversals) == 4 + 20 + 2 + 6 - 1 + 6 );
}
