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
      : m_wkt(wkt)
    {
    }

    bool next() override
    {
        static bool called = false;
        if (called) {
            return false;
        }

        called = true;
        return true;
    }

    GEOSGeometry* feature_geometry(const GEOSContextHandle_t& context) const override
    {
        return GEOSGeomFromWKT_r(context, m_wkt.c_str());
    }

    std::string feature_field(const std::string& field_name) const override
    {
        return field_name + "_value";
    }

    const std::string& id_field() const override
    {
        static std::string fid = "fid";
        return fid;
    }

  private:
    std::string m_wkt;
};

class TestWriter : public OutputWriter
{
  public:
    void write(const std::string& fid) override
    {
        for (const auto& op : m_ops) {
            op->set_result(*m_sr, fid, m_feature);
        }
    }

    void set_registry(const StatsRegistry* sr) override
    {
        m_sr = sr;
    }

    MapFeature m_feature;

  private:
    const StatsRegistry* m_sr;
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
