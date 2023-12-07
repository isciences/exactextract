#include <limits>

#include "catch.hpp"

#include "grid.h"
#include "matrix.h"
#include "raster.h"

using namespace exactextract;

template<typename T>
void
fill_sequential(Raster<T>& r)
{
    for (size_t i = 0; i < r.rows(); i++) {
        for (size_t j = 0; j < r.cols(); j++) {
            r(i, j) = i * r.cols() + j;
        }
    }
}

TEST_CASE("Iterator comparison")
{
    Raster<float> r{ { -180, -90, 180, 90 }, 180, 360 };
    fill_sequential(r);

    Raster<float> r2{ { -180, -90, 180, 90 }, 180, 360 };

    auto a = r.begin();
    auto b = r.begin();

    CHECK(a == b);
    CHECK(!(a != b));

    auto c = r2.begin();

    CHECK(a != c);
    CHECK(!(c == a));

    ++a;
    CHECK(a != b);
    CHECK(!(a == b));
}

TEST_CASE("Iterator operates rowwise")
{
    Raster<float> r{ { 0, 0, 8, 10 }, 10, 8 };
    fill_sequential(r);

    auto it = r.begin();
    CHECK(*it == 0);
    ++it;
    CHECK(*it == 1);

    std::vector<float> values(r.begin(), r.end());

    std::vector<float> expected(r.size());
    std::iota(expected.begin(), expected.end(), 0);

    CHECK(values == expected);
}
