#include "catch.hpp"
#include "gdal_feature.h"
#include "gdal_feature_unnester.h"
#include <gdal.h>

using namespace exactextract;

TEST_CASE("GDAL feature access", "[gdal]")
{
    OGRFeatureDefnH defn = OGR_FD_Create("test");

    OGRFieldDefnH int_field = OGR_Fld_Create("int_field", OFTInteger);
    OGR_FD_AddFieldDefn(defn, int_field);
    OGR_Fld_Destroy(int_field);

    OGRFieldDefnH int64_field = OGR_Fld_Create("int64_field", OFTInteger64);
    OGR_FD_AddFieldDefn(defn, int64_field);
    OGR_Fld_Destroy(int64_field);

    OGRFieldDefnH dbl_field = OGR_Fld_Create("dbl_field", OFTReal);
    OGR_FD_AddFieldDefn(defn, dbl_field);
    OGR_Fld_Destroy(dbl_field);

    OGRFieldDefnH str_field = OGR_Fld_Create("str_field", OFTString);
    OGR_FD_AddFieldDefn(defn, str_field);
    OGR_Fld_Destroy(str_field);

    OGRFieldDefnH int_v_field = OGR_Fld_Create("int_v_field", OFTIntegerList);
    OGR_FD_AddFieldDefn(defn, int_v_field);
    OGR_Fld_Destroy(int_v_field);

    OGRFieldDefnH int64_v_field = OGR_Fld_Create("int64_v_field", OFTInteger64List);
    OGR_FD_AddFieldDefn(defn, int64_v_field);
    OGR_Fld_Destroy(int64_v_field);

    OGRFieldDefnH dbl_v_field = OGR_Fld_Create("dbl_v_field", OFTRealList);
    OGR_FD_AddFieldDefn(defn, dbl_v_field);
    OGR_Fld_Destroy(dbl_v_field);

    GDALFeature feature(OGR_F_Create(defn));

    OGRGeometryH geom;
    std::string wkt("POINT (3 2)");
    char* wktp = (char*)wkt.c_str();
    OGR_G_CreateFromWkt(&wktp, nullptr, &geom);
    OGR_F_SetGeometryDirectly(feature.raw(), geom);

    SECTION("int field access")
    {
        feature.set("int_field", 123);
        CHECK(feature.get_int("int_field") == 123);
        feature.set("int_field", 456);
        CHECK(feature.get_int("int_field") == 456);
        CHECK(feature.field_type("int_field") == Feature::ValueType::INT);
    }

    SECTION("string field access")
    {
        feature.set("str_field", "abc");
        CHECK(feature.get_string("str_field") == "abc");
        feature.set("str_field", "def");
        CHECK(feature.get_string("str_field") == "def");
        CHECK(feature.field_type("str_field") == Feature::ValueType::STRING);
    }

    SECTION("int64 field access")
    {
        feature.set("int64_field", std::int64_t(5000000000));
        CHECK(feature.get_int64("int64_field") == std::int64_t(5000000000));
        feature.set("int64_field", std::int64_t(6000000000));
        CHECK(feature.get_int64("int64_field") == 6000000000);
        CHECK(feature.field_type("int64_field") == Feature::ValueType::INT64);
        CHECK_THROWS(feature.set("int64_field", static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 100));
    }

    SECTION("double field access")
    {
        feature.set("dbl_field", 123.456);
        CHECK(feature.get_double("dbl_field") == 123.456);
        CHECK(feature.field_type("dbl_field") == Feature::ValueType::DOUBLE);
    }

    SECTION("int array field access")
    {
        std::vector<std::int32_t> int_v{ 1, 2, 3 };
        feature.set("int_v_field", int_v);
        auto x = feature.get_integer_array("int_v_field");
        CHECK(x.size == 3);
        CHECK(x.data[0] == 1);
        CHECK(x.data[1] == 2);
        CHECK(x.data[2] == 3);
        CHECK(feature.field_type("int_v_field") == Feature::ValueType::INT_ARRAY);

        std::vector<std::int16_t> int16_v{ 4, 5, 6 };
        feature.set("int_v_field", int16_v);
        x = feature.get_integer_array("int_v_field");
        CHECK(x.size == 3);
        CHECK(x.data[0] == 4);
        CHECK(x.data[1] == 5);
        CHECK(x.data[2] == 6);
        CHECK(feature.field_type("int_v_field") == Feature::ValueType::INT_ARRAY);

        std::vector<std::int16_t> int8_v{ 7, 8, 9 };
        feature.set("int_v_field", int8_v);
        x = feature.get_integer_array("int_v_field");
        CHECK(x.size == 3);
        CHECK(x.data[0] == 7);
        CHECK(x.data[1] == 8);
        CHECK(x.data[2] == 9);
        CHECK(feature.field_type("int_v_field") == Feature::ValueType::INT_ARRAY);
    }

    SECTION("int64 array field access")
    {
        std::vector<std::int64_t> int64_v{ 5000000000, 6000000000 };
        feature.set("int64_v_field", int64_v);
        auto x = feature.get_integer64_array("int64_v_field");
        CHECK(x.size == 2);
        CHECK(x.data[0] == 5000000000);
        CHECK(x.data[1] == 6000000000);
        CHECK(feature.field_type("int64_v_field") == Feature::ValueType::INT64_ARRAY);
    }

    SECTION("double array field access")
    {
        std::vector<double> dbl_v{ 1.1, 2.2, 3.3 };
        feature.set("dbl_v_field", dbl_v);
        auto x = feature.get_double_array("dbl_v_field");
        CHECK(x.size == 3);
        CHECK(x.data[0] == 1.1);
        CHECK(x.data[1] == 2.2);
        CHECK(x.data[2] == 3.3);
        CHECK(feature.field_type("dbl_v_field") == Feature::ValueType::DOUBLE_ARRAY);
    }

    CHECK_THROWS(feature.get("does_not_exist"));

    CHECK(feature.geometry() != nullptr);

    SECTION("move construction and assignment")
    {
        feature.set("int_field", 999);

        OGRFeatureH ptr = feature.raw();

        GDALFeature f2 = std::move(feature);
        CHECK(f2.raw() == ptr);
        CHECK(f2.geometry() != nullptr);
        CHECK(f2.get_int("int_field") == 999);

        feature = std::move(f2);
        CHECK(feature.raw() == ptr);
        CHECK(feature.geometry() != nullptr);
        CHECK(feature.get_int("int_field") == 999);
    }
}

TEST_CASE("feature unnest", "[gdal]")
{
    OGRFeatureDefnH nested_defn = OGR_FD_Create("nested");
    OGRFeatureDefnH unnested_defn = OGR_FD_Create("unnested");
    OGR_FD_Reference(nested_defn);
    OGR_FD_Reference(unnested_defn);

    OGRFieldDefnH int_field = OGR_Fld_Create("int_field", OFTInteger);
    OGR_FD_AddFieldDefn(nested_defn, int_field);
    OGR_FD_AddFieldDefn(unnested_defn, int_field);
    OGR_Fld_Destroy(int_field);

    OGRFieldDefnH dbl_v_field = OGR_Fld_Create("dbl_field", OFTRealList);
    OGR_FD_AddFieldDefn(nested_defn, dbl_v_field);
    OGR_Fld_Destroy(dbl_v_field);

    OGRFieldDefnH dbl_field = OGR_Fld_Create("dbl_field", OFTReal);
    OGR_FD_AddFieldDefn(unnested_defn, dbl_field);
    OGR_Fld_Destroy(dbl_field);

    OGRFieldDefnH int64_v_field = OGR_Fld_Create("int64_field", OFTInteger64List);
    OGR_FD_AddFieldDefn(nested_defn, int64_v_field);
    OGR_Fld_Destroy(int64_v_field);

    OGRFieldDefnH int64_field = OGR_Fld_Create("int64_field", OFTInteger64);
    OGR_FD_AddFieldDefn(unnested_defn, int64_field);
    OGR_Fld_Destroy(int64_field);

    OGRFieldDefnH str_field = OGR_Fld_Create("str_field", OFTString);
    OGR_FD_AddFieldDefn(nested_defn, str_field);
    OGR_FD_AddFieldDefn(unnested_defn, str_field);
    OGR_Fld_Destroy(str_field);

    SECTION("ordinary usage")
    {
        GDALFeature feature(OGR_F_Create(nested_defn));

        feature.set("int_field", 3);
        feature.set("dbl_field", std::vector<double>{ 1.1, 2.2, 3.3 });
        feature.set("int64_field", std::vector<std::int64_t>{ 4, 5, 6 });
        feature.set("str_field", "a");

        GDALFeatureUnnester unnester(feature, unnested_defn);
        unnester.unnest();
        const auto& unnested = unnester.features();

        CHECK(unnested.size() == 3);

        CHECK(unnested[0]->get_int("int_field") == 3);
        CHECK(unnested[1]->get_int("int_field") == 3);
        CHECK(unnested[2]->get_int("int_field") == 3);

        CHECK(unnested[0]->get_double("dbl_field") == 1.1);
        CHECK(unnested[1]->get_double("dbl_field") == 2.2);
        CHECK(unnested[2]->get_double("dbl_field") == 3.3);

        CHECK(unnested[0]->get_int64("int64_field") == 4);
        CHECK(unnested[1]->get_int64("int64_field") == 5);
        CHECK(unnested[2]->get_int64("int64_field") == 6);

        CHECK(unnested[0]->get_string("str_field") == "a");
        CHECK(unnested[1]->get_string("str_field") == "a");
        CHECK(unnested[2]->get_string("str_field") == "a");
    }

    SECTION("zero-length arrays")
    {
        GDALFeature feature(OGR_F_Create(nested_defn));

        feature.set("int_field", 1);
        feature.set("int64_field", std::vector<std::int64_t>{});
        feature.set("dbl_field", std::vector<double>{});
        feature.set("str_field", "a");

        GDALFeatureUnnester unnester(feature, unnested_defn);
        unnester.unnest();
        const auto& unnested = unnester.features();

        CHECK(unnested.size() == 0);
    }

    SECTION("mixed-length arrays")
    {
        GDALFeature feature(OGR_F_Create(nested_defn));

        feature.set("int_field", 1);
        feature.set("int64_field", std::vector<std::int64_t>{ 1, 2, 3 });
        feature.set("dbl_field", std::vector<double>{ 4.4, 5.5 });
        feature.set("str_field", "a");

        GDALFeatureUnnester unnester(feature, unnested_defn);

        CHECK_THROWS(unnester.unnest());
    }

    OGR_FD_Release(nested_defn);
    OGR_FD_Release(unnested_defn);
}
