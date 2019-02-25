#ifndef EXACTEXTRACT_RASTER_SEQUENTIAL_PROCESSOR_H
#define EXACTEXTRACT_RASTER_SEQUENTIAL_PROCESSOR_H

#include <algorithm>
#include <fstream>
#include <unordered_set>
#include <set>
#include <vector>
#include "raster_stats.h"
#include "operation.h"
#include "geos_utils.h"
#include "gdal_dataset_wrapper.h"
#include "gdal_raster_wrapper.h"
#include "grid.h"
#include "stats_registry.h"

#include "output_writer.h"

namespace exactextract {


    using Feature=std::pair<std::string, geom_ptr_r>;

    template<typename T>
    Grid<bounded_extent> common_grid(T begin, T end) {
        if (begin == end) {
            return Grid<bounded_extent>::make_empty();
        } else if (std::next(begin) == end) {
            return begin->grid();
        }
        return std::accumulate(
                std::next(begin),
                end,
                begin->grid(),
                [](auto& acc, auto& op) {
                    return acc.common_grid(op.grid());
                });
    }

    class Processor {

    public:
        Processor(GDALDatasetWrapper & ds, OutputWriter & out, const std::string id_field) :
        m_shp{ds},
        m_reg{},
        m_output{out},
        m_field_name{id_field}
        {
            m_geos_context = initGEOS_r(nullptr, nullptr);
            m_output.set_registry(&m_reg);
        }

        virtual ~Processor() {
            finishGEOS_r(m_geos_context);
        }

        void add_operation(const Operation & op) {
            m_operations.push_back(op);
        }

    protected:

        void progress(const std::string & name) {
            std::cout << std::endl << "Processing " << name << std::flush;
        }

        void progress() {
            std::cout << "." << std::flush;
        }

        GEOSContextHandle_t m_geos_context;
        StatsRegistry m_reg;

        OutputWriter& m_output;

        GDALDatasetWrapper& m_shp;

        std::string m_field_name;

        bool store_values=false;

        std::string filter;

        std::vector<Operation> m_operations;

        //std::vector<GDALRasterWrapper*> m_values;
        //std::vector<GDALRasterWrapper*> m_weights;

        size_t max_cells_in_memory = 1000000L;
    };

#if 0

    class RasterSequentialProcessor : public Processor {
    public:

        void read_features() {
            while (m_shp.next()) {
                Feature feature = std::make_pair(
                        m_shp.feature_field(m_field_name),
                        geos_ptr(m_geos_context, m_shp.feature_geometry(m_geos_context)));
                features.push_back(std::move(feature));
            }
        }

        void populate_index() {
            for (const Feature& f : features) {
                // TODO compute envelope of dataset, and crop raster by that extent before processing?
                GEOSSTRtree_insert_r(m_geos_context, feature_tree.get(), f.second.get(), (void *) &f);
            }
        }

        void process() {
            read_features();
            populate_index();

            // FIXME actually get comon grid if needed
            Grid<bounded_extent> common_grid = m_values[0]->grid();

            for (const auto &subgrid : subdivide(common_grid, max_cells_in_memory)) {
                std::vector<const Feature *> hits;

                auto query_rect = geos_make_box_polygon(m_geos_context, subgrid.extent());

                GEOSSTRtree_query_r(m_geos_context, feature_tree.get(), query_rect.get(), [](void *hit, void *userdata) {
                    auto feature = static_cast<const Feature *>(hit);
                    auto vec = static_cast<std::vector<const Feature *> *>(userdata);

                    vec->push_back(feature);
                }, &hits);

                if (!hits.empty()) {
                    for (const auto & v : m_values) {
                        Raster<double> values_cropped = v->read_box(subgrid.extent());

                        if (m_weights.empty()) {
                            for (const auto& feature : hits) {
                                Raster<float> coverage = raster_cell_intersection(subgrid, m_geos_context, feature->second.get());

                                auto rs = m_reg.stats(feature->first, v->name());
                                rs.process(coverage, values_cropped);
                            }
                        } else {
                            for (const auto & w : m_weights) {
                                Raster<double> weights_cropped = w->read_box(subgrid.extent());

                                for (const auto& feature : hits) {
                                    Raster<float> coverage = raster_cell_intersection(subgrid, m_geos_context, feature->second.get());

                                    auto rs = m_reg.stats(feature->first, v->name(), w->name());
                                    rs.process(coverage, values_cropped, weights_cropped);
                                }
                            }
                        }

                    }
                }
            }
        }

    private:

        std::vector<Feature> features;
        tree_ptr_r feature_tree{GEOSSTRtree_create_r(m_geos_context, 10)};
    };
#endif

    class FeatureSequentialProcessor : public Processor {
    public:
        using Processor::Processor;

        void process() {
            for (const auto& op : m_operations) {
                m_output.add_operation(op);
            }

            while (m_shp.next()) {
                std::string name{m_shp.feature_field(m_field_name)};
                auto geom = geos_ptr(m_geos_context, m_shp.feature_geometry(m_geos_context));

                progress(name);

                Box feature_bbox = exactextract::geos_get_box(m_geos_context, geom.get());

                auto grid = common_grid(m_operations.begin(), m_operations.end());

                if (feature_bbox.intersects(grid.extent())) {
                    // Crop grid to portion overlapping feature
                    auto cropped_grid = grid.crop(feature_bbox);

                    for (const auto &subgrid : subdivide(cropped_grid, max_cells_in_memory)) {
                        Raster<float> coverage = raster_cell_intersection(subgrid, m_geos_context,
                                                                          geom.get());


                        std::set<std::pair<GDALRasterWrapper*, GDALRasterWrapper*>> processed;

                        for (const auto &op : m_operations) {
                            // TODO avoid reading same values, weights multiple times. Just use a map?

                            // Avoid processing same values/weights for different stats
                            if (processed.find(std::make_pair(op.weights, op.values)) != processed.end())
                                continue;

                            Raster<double> values = op.values->read_box(subgrid.extent());

                            if (op.weighted()) {
                                Raster<double> weights = op.weights->read_box(subgrid.extent());

                                m_reg.stats(name, op).process(coverage, values, weights);
                            } else {
                                m_reg.stats(name, op).process(coverage, values);
                            }

                            progress();
                        }
                    }
                }

                m_output.write(name);
                m_reg.flush_feature(name);
            }
        }
    };
};

#endif //EXACTEXTRACT_RASTER_SEQUENTIAL_PROCESSOR_H
