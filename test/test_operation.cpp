#include "catch.hpp"

#include "feature_sequential_processor.h"
#include "feature_source.h"
#include "map_feature.h"
#include "operation.h"
#include "output_writer.h"
#include "raster.h"
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
    WKTFeatureSource(const std::string& wkt)
      : m_count(0), m_wkt(wkt), m_context(initGEOS_r(nullptr, nullptr))
    {
        m_feature.set("fid", "myfid");
        m_feature.set_geometry(geos_ptr(m_context, GEOSGeomFromWKT_r(m_context, m_wkt.c_str())));
    }

    ~WKTFeatureSource() {
        finishGEOS_r(m_context);
    }

    bool next() override
    {
        return m_count++ < 1;
    }

    const Feature& feature() const override {
        return m_feature;
    }

    const std::string& id_field() const override
    {
        static std::string fid = "fid";
        return fid;
    }

  private:
    std::size_t m_count;
    std::string m_wkt;
    GEOSContextHandle_t m_context;
    MapFeature m_feature;
};

class TestWriter : public OutputWriter
{
  public:
    std::unique_ptr<Feature> create_feature() override {
        return std::make_unique<MapFeature>();
    }

    void write(const Feature& f) override {
        m_feature = MapFeature(f);
    }

    MapFeature m_feature;
};

TEST_CASE("frac sets appropriate column names", "[operation]")
{
    Grid<bounded_extent> ex{ { 0, 0, 3, 3 }, 1, 1 }; // 3x3 grid
    Matrix<double> values{ { { 9, 1, 1 },
                             { 2, 2, 2 },
                             { 3, 3, 3 } } };

    Raster<double> value_rast(std::move(values), ex.extent());
    MemoryRasterSource<double> value_src(value_rast);

    WKTFeatureSource ds("POLYGON ((0.5 0.5, 2.5 0.5, 2.5 2.5, 0.5 0.5))");

    std::vector<std::unique_ptr<Operation>> ops;
    ops.emplace_back(std::make_unique<Operation>("frac", "x", &value_src, nullptr));

    TestWriter writer;

    FeatureSequentialProcessor fsp(ds, writer, std::move(ops));

    fsp.process();

    const MapFeature& f = writer.m_feature;

    CHECK(f.get<float>("frac_1") == 0.0625f);
    CHECK(f.get<float>("frac_2") == 0.50f);
    CHECK(f.get<float>("frac_3") == 0.4375f);

    CHECK_THROWS(f.get<float>("frac_9"));
}

TEST_CASE("weighted_frac sets appropriate column names", "[operation]")
{
    Grid<bounded_extent> ex{ { 0, 0, 3, 3 }, 1, 1 }; // 3x3 grid
    Matrix<double> values{ { { 9, 1, 1 },
                             { 2, 2, 2 },
                             { 3, 3, 3 } } };

    Raster<double> value_rast(std::move(values), ex.extent());
    MemoryRasterSource<double> value_src(value_rast);

    Matrix<double> weights{ { { 0, 0, 0},
                              { 0, 2, 0},
                              { 0, 0, 1} } };
    Raster<double> weight_rast(std::move(weights), ex.extent());
    MemoryRasterSource<double> weight_src(weight_rast);

    WKTFeatureSource ds("POLYGON ((0 0, 3 0, 3 3, 0 0))");

    std::vector<std::unique_ptr<Operation>> ops;
    ops.emplace_back(std::make_unique<Operation>("weighted_frac", "x", &value_src, &weight_src));

    TestWriter writer;

    FeatureSequentialProcessor fsp(ds, writer, std::move(ops));

    fsp.process();

    const MapFeature& f = writer.m_feature;

    CHECK(f.get<float>("weighted_frac_1") == 0.00f);
    CHECK(f.get<float>("weighted_frac_2") == 0.50f);
    CHECK(f.get<float>("weighted_frac_3") == 0.50f);

    CHECK_THROWS(f.get<float>("weighted_frac_9"));
}
