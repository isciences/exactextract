#include "catch.hpp"

#include "feature_sequential_processor.h"
#include "feature_source.h"
#include "map_feature.h"
#include "memory_raster_source.h"
#include "operation.h"
#include "output_writer.h"
#include "raster.h"
#include "raster_sequential_processor.h"
#include "raster_source.h"

using namespace exactextract;

class WKTFeatureSource : public FeatureSource
{
  public:
    WKTFeatureSource()
      : m_count(0)
    {
    }

    void reset()
    {
        m_count = 0;
    }

    void add_feature(MapFeature m)
    {
        m_features.emplace_back(std::move(m));
    }

    bool next() override
    {
        return m_count++ < m_features.size();
    }

    const Feature& feature() const override
    {
        return m_features[m_count - 1];
    }

  private:
    std::size_t m_count;
    std::vector<MapFeature> m_features;
};

class TestWriter : public OutputWriter
{
  public:
    std::unique_ptr<Feature> create_feature() override
    {
        return std::make_unique<MapFeature>();
    }

    void write(const Feature& f) override
    {
        m_feature = MapFeature(f);
    }

    MapFeature m_feature;
};

static GEOSContextHandle_t
init_geos()
{
    static GEOSContextHandle_t context = nullptr;

    if (context == nullptr) {
        context = initGEOS_r(nullptr, nullptr);
    }

    return context;
}

TEMPLATE_TEST_CASE("raster value type is mapped to appropriate field type", "[operation]", double, std::int32_t, std::int64_t)
{
    GEOSContextHandle_t context = init_geos();

    Grid<bounded_extent> ex{ { 0, 0, 3, 3 }, 1, 1 }; // 3x3 grid
    Matrix<TestType> values{ { { 1, 1, 1 },
                               { 2, 2, 2 },
                               { 2, 2, 2 } } };
    auto value_rast = std::make_unique<Raster<TestType>>(std::move(values), ex.extent());
    MemoryRasterSource value_src(std::move(value_rast));

    WKTFeatureSource ds;
    MapFeature mf;
    mf.set_geometry(geos_ptr(context, GEOSGeomFromWKT_r(context, "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2.5, 0.5 0.5))")));
    ds.add_feature(std::move(mf));

    TestWriter writer;
    FeatureSequentialProcessor fsp(ds, writer);
    auto op = Operation::create("mode", "mode", &value_src, nullptr);
    fsp.add_operation(*op);
    fsp.process();

    const MapFeature& f = writer.m_feature;

    CHECK(op->result_type() == typeid(TestType));

    const auto& pix_typ = typeid(TestType);
    if (pix_typ == typeid(float) || pix_typ == typeid(double)) {
        CHECK(f.field_type("mode") == typeid(double));
    } else if (pix_typ == typeid(std::int32_t) || pix_typ == typeid(std::int64_t)) {
        CHECK(f.field_type("mode") == typeid(std::int32_t));
    }
}

TEMPLATE_TEST_CASE("frac values correspond to entries in unique()", "[operation]", std::int8_t, double)
{
    GEOSContextHandle_t context = init_geos();

    Grid<bounded_extent> ex{ { 0, 0, 3, 3 }, 1, 1 }; // 3x3 grid
    Matrix<TestType> values{ { { 9, 1, 1 },
                               { 2, 2, 2 },
                               { 3, 3, 3 } } };

    auto value_rast = std::make_unique<Raster<TestType>>(std::move(values), ex.extent());
    MemoryRasterSource value_src(std::move(value_rast));

    WKTFeatureSource ds;
    MapFeature mf;
    mf.set("fid", "15");
    mf.set_geometry(geos_ptr(context, GEOSGeomFromWKT_r(context, "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2.5, 0.5 0.5))")));
    ds.add_feature(std::move(mf));

    std::vector<std::unique_ptr<Operation>> ops;
    ops.emplace_back(Operation::create("unique", "unique", &value_src, nullptr));
    ops.emplace_back(Operation::create("frac", "frac", &value_src, nullptr));

    TestWriter writer;

    FeatureSequentialProcessor fsp(ds, writer);
    for (const auto& op : ops) {
        fsp.add_operation(*op);
    }

    fsp.process();

    const MapFeature& f = writer.m_feature;

    std::map<TestType, double> fracs;

    auto frac = f.get_double_array("frac");
    if constexpr (std::is_floating_point<TestType>::value) {
        auto unique = f.get_double_array("unique");

        CHECK(unique.size == frac.size);

        for (std::size_t i = 0; i < unique.size; i++) {
            fracs[unique.data[i]] = frac.data[i];
        }
    } else {
        auto unique = f.get_integer_array("unique");

        CHECK(unique.size == frac.size);

        for (std::size_t i = 0; i < unique.size; i++) {
            fracs[unique.data[i]] = frac.data[i];
        }
    }

    CHECK(fracs.at(1.0) == 0.0625);
    CHECK(fracs.at(2.0) == 0.50);
    CHECK(fracs.at(3.0) == 0.4375);
}

TEST_CASE("error thrown if no weights provided for weighted operation", "[operation]")
{
    Grid<bounded_extent> ex{ { 0, 0, 3, 3 }, 1, 1 }; // 3x3 grid
    Matrix<double> values{ { { 9, 1, 1 },
                             { 2, 2, 2 },
                             { 3, 3, 3 } } };

    auto value_rast = std::make_unique<Raster<double>>(std::move(values), ex.extent());
    MemoryRasterSource value_src(std::move(value_rast));

    CHECK_THROWS(Operation::create("weighted_mean", "test", &value_src, nullptr));
}

TEMPLATE_TEST_CASE("no error if feature does not intersect raster", "[processor]", FeatureSequentialProcessor, RasterSequentialProcessor)
{
    GEOSContextHandle_t context = init_geos();

    Grid<bounded_extent> ex{ { 100, 100, 103, 103 }, 1, 1 }; // 3x3 grid
    Matrix<double> values{ { { 9, 1, 1 },
                             { 2, 2, 2 },
                             { 3, 3, 3 } } };

    auto value_rast = std::make_unique<Raster<double>>(std::move(values), ex.extent());
    MemoryRasterSource value_src(std::move(value_rast));

    WKTFeatureSource ds;
    MapFeature mf;
    mf.set("fid", "15");
    mf.set_geometry(geos_ptr(context, GEOSGeomFromWKT_r(context, "POLYGON ((0 0, 3 0, 3 3, 0 0))")));
    ds.add_feature(std::move(mf));

    TestWriter writer;

    TestType processor(ds, writer);
    auto count = Operation::create("count", "count", &value_src, nullptr);
    auto median = Operation::create("median", "median", &value_src, nullptr);
    processor.add_operation(*count);
    processor.add_operation(*median);
    processor.process();

    const MapFeature& f = writer.m_feature;
    CHECK(f.get<double>("count") == 0);
    CHECK(std::isnan(f.get<double>("median")));
}

TEMPLATE_TEST_CASE("correct result for feature partially intersecting raster", "[processor]", FeatureSequentialProcessor, RasterSequentialProcessor)
{
    GEOSContextHandle_t context = init_geos();

    Grid<bounded_extent> ex{ { 0, 0, 3, 3 }, 1, 1 }; // 3x3 grid
    Matrix<double> values{ { { 1, 2, 3 },
                             { 4, 5, 6 },
                             { 7, 8, 9 } } };
    auto value_rast = std::make_unique<Raster<double>>(std::move(values), ex.extent());
    MemoryRasterSource value_src(std::move(value_rast));

    WKTFeatureSource ds;
    MapFeature mf;
    mf.set("fid", "15");
    mf.set_geometry(geos_ptr(context, GEOSGeomFromWKT_r(context, "POLYGON ((2 2, 100 2, 100 100, 2 100, 2 2))")));
    ds.add_feature(std::move(mf));

    TestWriter writer;

    TestType processor(ds, writer);
    auto count = Operation::create("count", "count", &value_src, nullptr);
    auto median = Operation::create("median", "median", &value_src, nullptr);
    processor.add_operation(*count);
    processor.add_operation(*median);
    processor.process();

    const MapFeature& f = writer.m_feature;
    CHECK(f.get<double>("count") == 1);
    CHECK(f.get<double>("median") == 3);
}

TEMPLATE_TEST_CASE("include_col and include_geom work as expected", "[processor]", FeatureSequentialProcessor, RasterSequentialProcessor)
{
    GEOSContextHandle_t context = init_geos();

    Grid<bounded_extent> ex{ { 0, 0, 3, 3 }, 1, 1 }; // 3x3 grid
    Matrix<double> values{ { { 1, 2, 3 },
                             { 4, 5, 6 },
                             { 7, 8, 9 } } };
    auto value_rast = std::make_unique<Raster<double>>(std::move(values), ex.extent());
    MemoryRasterSource value_src(std::move(value_rast));

    WKTFeatureSource ds;
    MapFeature mf;
    mf.set("fid", "15");
    mf.set("type", 13);
    mf.set_geometry(geos_ptr(context, GEOSGeomFromWKT_r(context, "POLYGON ((0 0, 2 0, 2 2, 0 2, 0 0))")));
    ds.add_feature(std::move(mf));

    TestWriter writer;

    TestType processor(ds, writer);
    auto count = Operation::create("count", "count", &value_src, nullptr);
    processor.add_operation(*count);
    processor.include_col("fid");
    processor.include_col("type");
    processor.include_geometry();
    processor.process();

    ds.reset();
    ds.next();

    const MapFeature& f = writer.m_feature;
    CHECK(f.get<double>("count") == 4.0);
    CHECK(f.get<std::string>("fid") == "15");
    CHECK(f.get<std::int32_t>("type") == 13);
    CHECK(GEOSEquals_r(context, f.geometry(), ds.feature().geometry()) == 1);
}

TEST_CASE("Operation arguments", "[operation]")
{
    MemoryRasterSource mrs{ std::make_unique<Raster<float>>(Raster<float>::make_empty()) };

    SECTION("successful arg parsing")
    {
        Operation::ArgMap args{ { "q", "0.05" } };

        auto op = Operation::create("quantile", "fizz", &mrs, nullptr, args);
        CHECK(op->name == "fizz_5");

        // args are not modified
        CHECK(args.find("q") != args.end());
    }

    SECTION("error if not all args are consumed")
    {
        Operation::ArgMap args{ { "q", "0.05" }, { "qq", "0.15" } };
        CHECK_THROWS_WITH(Operation::create("quantile", "quantile", &mrs, nullptr, args), Catch::StartsWith("Unexpected argument"));

        // args are not modified
        CHECK(args.find("q") != args.end());
        CHECK(args.find("qq") != args.end());
    }

    SECTION("error if arg cannot be parsed")
    {
        Operation::ArgMap args{ { "q", "chicken" } };
        CHECK_THROWS_WITH(Operation::create("quantile", "quantile", &mrs, nullptr, args), Catch::StartsWith("Failed to parse"));
    }

    SECTION("error if arg is invalid")
    {
        Operation::ArgMap args{ { "q", "5" } };
        CHECK_THROWS_WITH(Operation::create("quantile", "quantile", &mrs, nullptr, args), Catch::Contains("must be between 0 and 1"));
    }

    SECTION("error if required arg is missing")
    {
        Operation::ArgMap args;
        CHECK_THROWS_WITH(Operation::create("quantile", "quantile", &mrs, nullptr, args), Catch::StartsWith("Missing required argument"));
    }
}