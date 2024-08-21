#include "catch.hpp"

#include "cell.h"

using namespace exactextract;

TEST_CASE("Test multi-traversal area calculations", "[cell]")
{
    GEOSContextHandle_t context = initGEOS_r(nullptr, nullptr);

    Cell c{ 0, 0, 20, 20 };

    // Grab lower-left hand corner
    c.take({ 5, 0 });
    c.take({ 5, 5 });
    c.take({ 0, 5 });
    c.force_exit();

    CHECK(c.last_traversal().traversed());
    CHECK(c.covered_fraction(context) == 25 / 400.0);

    // Grab right-hand side
    c.take({ 13, 20 });
    c.take({ 13, 0 });
    c.force_exit();

    CHECK(c.last_traversal().traversed());
    CHECK(c.covered_fraction(context) == (25 + 140) / 400.0);

    // Carve out a piece from right-hand side
    c.take({ 20, 5 });
    c.take({ 18, 5 });
    c.take({ 18, 10 });
    c.take({ 20, 10 });
    c.force_exit();

    CHECK(c.last_traversal().traversed());
    CHECK(c.covered_fraction(context) == (25 + 140 - 10) / 400.0);

    finishGEOS_r(context);
}
