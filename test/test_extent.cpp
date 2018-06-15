#include "catch.hpp"

#include "extent.h"

using namespace exactextract;

TEST_CASE( "Extent dimensions calculated correctly", "[extent]") {
    Extent ex{-180, -90, 180, 90, 0.5, 0.5};

    CHECK( ex.rows() == 360);
    CHECK( ex.cols() == 720);
    }

    TEST_CASE( "Extent dimension robustness", "[extent]") {
    Extent ex{8.5, 1.6, 16.2, 13.1, 0.1, 0.1};

    CHECK(ex.cols() == 77);
    CHECK(ex.rows() == 115);
}

TEST_CASE( "Extent index lookups are correct", "[extent]") {
    Extent ex{-180, -90, 180, 90, 1.0, 0.5};

    CHECK( ex.get_row(90) == 0 );
    CHECK( ex.get_row(-89.50000001) == 359 );
    CHECK( ex.get_row(-89.5) == 359 );
    CHECK( ex.get_row(-90) == 359 );

    CHECK_THROWS( ex.get_row(-90.00000001) );
    CHECK_THROWS( ex.get_row( 90.00000001) );

    CHECK( ex.get_column(-180) == 0 );
    CHECK( ex.get_column(-179.000001) == 0 );
    CHECK( ex.get_column(-179) == 1 );
    CHECK( ex.get_column(179) == 359 );
    CHECK( ex.get_column(180) == 359 );

    CHECK_THROWS( ex.get_column(-180.0000001) );
    CHECK_THROWS( ex.get_column( 180.0000001) );
}

TEST_CASE( "Extent shrink works correctly", "[extent]") {
    Extent ex{-180, -90, 180, 90, 1, 0.5};

    Extent ex2 = ex.shrink_to_fit(-44.3, -21.4, 18.3, 88.2);

    CHECK( ex2.xmin == -45 );
    CHECK( ex2.xmax == 19 );
    CHECK( ex2.ymin == -21.5 );
    CHECK( ex2.ymax == 88.5 );
    CHECK( ex2.dx == ex.dx );
    CHECK( ex2.dy == ex.dy );
}

TEST_CASE("Repeated shrink has no effect", "[extent]") {
    Extent ex{-180.5, -90, 180, 90, 0.1, 0.1};

    double x0 = 8.532812500000006,
            y0 = 1.6762207031249972,
            x1 = 16.183398437500017,
            y1 = 13.078515624999994;

    Extent ex2 = ex.shrink_to_fit(x0, y0, x1, y1);
    Extent ex3 = ex2.shrink_to_fit(x0, y0, x1, y1);

    CHECK( ex2.rows() == ex3.rows() );
    CHECK( ex2.cols() == ex3.cols() );
}

TEST_CASE("Shrink robustness", "[extent]") {
    Extent ex{-180.5, -90, 180, 90, 0.5, 0.5};

    double x0 = -1.0000000000000142,
            y0 = 8.141666666665664,
            x1 = 0.08749999999993818,
            y1 = 9.904166666665645;

    Extent ex2 = ex.shrink_to_fit(x0, y0, x1, y1);

    CHECK(x0 >= ex2.xmin);
    CHECK(x1 <= ex2.xmax);
    CHECK(y0 >= ex2.ymin);
    CHECK(y1 <= ex2.ymax);
}

TEST_CASE("Shrink robustness (2)", "[extent]") {
    Extent ex{-180.5, -90.5, 180.5, 90.5, 0.25, 0.25};

    double x0 = 129.75833333333242,
            y0 = -1.2541666666666238,
            x1 = 129.7624999999993,
            y1 = -1.2499999999999964;

    Extent ex2 = ex.shrink_to_fit(x0, y0, x1, y1);

    CHECK(x0 >= ex2.xmin);
    CHECK(x1 <= ex2.xmax);
    CHECK(y0 >= ex2.ymin);
    CHECK(y1 <= ex2.ymax);
}

