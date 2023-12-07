#include "catch.hpp"

#include "feature_sequential_processor.h"
#include "feature_source.h"
#include "map_feature.h"
#include "operation.h"
#include "output_writer.h"
#include "raster.h"
#include "raster_sequential_processor.h"
#include "raster_source.h"

using namespace exactextract;

template<typename T>
class MemoryRasterSource : public RasterSource
{
  public:
    MemoryRasterSource(const AbstractRaster<T>& rast)
      : m_rast(rast)
    {
    }

    std::unique_ptr<AbstractRaster<T>> read_box(const Box& b) override
    {
        if (b != m_rast.grid().extent()) {
            throw std::runtime_error("Unexpected extent.");
        }

        return std::make_unique<RasterView<T>>(m_rast, m_rast.grid());
    }

    const Grid<bounded_extent>& grid() const override
    {
        return m_rast.grid();
    }

  private:
    const AbstractRaster<T>& m_rast;
};

class WKTFeatureSource : public FeatureSource
{
  public:
    WKTFeatureSource()
      : m_count(0)
    {
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

    const std::string& id_field() const override
    {
        static std::string fid = "fid";
        return fid;
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

TEST_CASE("frac sets appropriate column names", "[operation]")
{
    GEOSContextHandle_t context = init_geos();

    Grid<bounded_extent> ex{ { 0, 0, 3, 3 }, 1, 1 }; // 3x3 grid
    Matrix<double> values{ { { 9, 1, 1 },
                             { 2, 2, 2 },
                             { 3, 3, 3 } } };

    Raster<double> value_rast(std::move(values), ex.extent());
    MemoryRasterSource<double> value_src(value_rast);

    WKTFeatureSource ds;
    MapFeature mf;
    mf.set("fid", "15");
    mf.set_geometry(geos_ptr(context, GEOSGeomFromWKT_r(context, "POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2.5, 0.5 0.5))")));
    ds.add_feature(std::move(mf));

    std::vector<std::unique_ptr<Operation>> ops;
    ops.emplace_back(std::make_unique<Operation>("frac", "x", &value_src, nullptr));

    TestWriter writer;

    FeatureSequentialProcessor fsp(ds, writer);
    for (const auto& op : ops) {
        fsp.add_operation(*op);
    }

    fsp.process();

    const MapFeature& f = writer.m_feature;

    CHECK(f.get<double>("frac_1") == 0.0625);
    CHECK(f.get<double>("frac_2") == 0.50);
    CHECK(f.get<double>("frac_3") == 0.4375);

    CHECK_THROWS(f.get<double>("frac_9"));
}

TEST_CASE("weighted_frac sets appropriate column names", "[operation]")
{
    GEOSContextHandle_t context = init_geos();

    Grid<bounded_extent> ex{ { 0, 0, 3, 3 }, 1, 1 }; // 3x3 grid
    Matrix<double> values{ { { 9, 1, 1 },
                             { 2, 2, 2 },
                             { 3, 3, 3 } } };

    Raster<double> value_rast(std::move(values), ex.extent());
    MemoryRasterSource<double> value_src(value_rast);

    Matrix<double> weights{ { { 0, 0, 0 },
                              { 0, 2, 0 },
                              { 0, 0, 1 } } };
    Raster<double> weight_rast(std::move(weights), ex.extent());
    MemoryRasterSource<double> weight_src(weight_rast);

    WKTFeatureSource ds;
    MapFeature mf;
    mf.set("fid", 15);
    mf.set_geometry(geos_ptr(context, GEOSGeomFromWKT_r(context, "POLYGON ((0 0, 3 0, 3 3, 0 0))")));
    ds.add_feature(std::move(mf));

    std::vector<std::unique_ptr<Operation>> ops;
    ops.emplace_back(std::make_unique<Operation>("weighted_frac", "x", &value_src, &weight_src));

    TestWriter writer;

    FeatureSequentialProcessor fsp(ds, writer);
    for (const auto& op : ops) {
        fsp.add_operation(*op);
    }

    fsp.process();

    const MapFeature& f = writer.m_feature;

    CHECK(f.get<double>("weighted_frac_1") == 0.00f);
    CHECK(f.get<double>("weighted_frac_2") == 0.50f);
    CHECK(f.get<double>("weighted_frac_3") == 0.50f);

    CHECK_THROWS(f.get<double>("weighted_frac_9"));
}

TEST_CASE("error thrown if no weights provided for weighted operation", "[operation]")
{
    Grid<bounded_extent> ex{ { 0, 0, 3, 3 }, 1, 1 }; // 3x3 grid
    Matrix<double> values{ { { 9, 1, 1 },
                             { 2, 2, 2 },
                             { 3, 3, 3 } } };

    Raster<double> value_rast(std::move(values), ex.extent());
    MemoryRasterSource<double> value_src(value_rast);

    CHECK_THROWS(Operation("weighted_mean", "test", &value_src, nullptr));
}

TEMPLATE_TEST_CASE("no error if feature does not intersect raster", "[processor]", FeatureSequentialProcessor, RasterSequentialProcessor)
{
    GEOSContextHandle_t context = init_geos();

    Grid<bounded_extent> ex{ { 100, 100, 103, 103 }, 1, 1 }; // 3x3 grid
    Matrix<double> values{ { { 9, 1, 1 },
                             { 2, 2, 2 },
                             { 3, 3, 3 } } };

    Raster<double> value_rast(std::move(values), ex.extent());
    MemoryRasterSource<double> value_src(value_rast);

    WKTFeatureSource ds;
    MapFeature mf;
    mf.set("fid", "15");
    mf.set_geometry(geos_ptr(context, GEOSGeomFromWKT_r(context, "POLYGON ((0 0, 3 0, 3 3, 0 0))")));
    ds.add_feature(std::move(mf));

    TestWriter writer;

    TestType processor(ds, writer);
    processor.add_operation(Operation("count", "count", &value_src, nullptr));
    processor.add_operation(Operation("median", "median", &value_src, nullptr));
    processor.process();

    const MapFeature& f = writer.m_feature;
    CHECK(f.get<double>("count") == 0);
    CHECK(std::isnan(f.get<double>("median")));
}
