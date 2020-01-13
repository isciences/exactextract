// Copyright (c) 2019-2020 ISciences, LLC.
// All rights reserved.
//
// This software is licensed under the Apache License, Version 2.0 (the "License").
// You may not use this file except in compliance with the License. You may
// obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "raster_sequential_processor.h"

#include <map>
#include <memory>
#include <set>

namespace exactextract {

    void RasterSequentialProcessor::read_features() {
        while (m_shp.next()) {
            Feature feature = std::make_pair(
                    m_shp.feature_field(m_shp.id_field()),
                    geos_ptr(m_geos_context, m_shp.feature_geometry(m_geos_context)));
            m_features.push_back(std::move(feature));
        }
    }

    void RasterSequentialProcessor::populate_index() {
        for (const Feature& f : m_features) {
            // TODO compute envelope of dataset, and crop raster by that extent before processing?
            GEOSSTRtree_insert_r(m_geos_context, m_feature_tree.get(), f.second.get(), (void *) &f);
        }
    }

    void RasterSequentialProcessor::process() {
        read_features();
        populate_index();

        for (const auto& op : m_operations) {
            m_output.add_operation(op);
        }

        auto grid = common_grid(m_operations.begin(), m_operations.end());

        for (const auto &subgrid : subdivide(grid, m_max_cells_in_memory)) {
            std::vector<const Feature *> hits;

            auto query_rect = geos_make_box_polygon(m_geos_context, subgrid.extent());

            GEOSSTRtree_query_r(m_geos_context, m_feature_tree.get(), query_rect.get(), [](void *hit, void *userdata) {
                auto feature = static_cast<const Feature *>(hit);
                auto vec = static_cast<std::vector<const Feature *> *>(userdata);

                vec->push_back(feature);
            }, &hits);

            std::map<RasterSource*, std::unique_ptr<AbstractRaster<double>>> raster_values;

            for (const auto &f : hits) {
                std::unique_ptr<Raster<float>> coverage;
                std::set<std::pair<RasterSource*, RasterSource*>> processed;

                for (const auto &op : m_operations) {
                    // Avoid processing same values/weights for different stats
                    auto key = std::make_pair(op.weights, op.values);
                    if (processed.find(key) != processed.end()) {
                        continue;
                    } else {
                        processed.insert(key);
                    }

                    if (!op.values->grid().extent().contains(subgrid.extent())) {
                        continue;
                    }

                    if (op.weighted() && !op.weights->grid().extent().contains(subgrid.extent())) {
                        continue;
                    }

                    // Lazy-initialize coverage
                    if (coverage == nullptr) {
                        coverage = std::make_unique<Raster<float>>(
                                raster_cell_intersection(subgrid, m_geos_context, f->second.get()));
                    }

                    // FIXME need to ensure that no values are read from a raster that have already been read.
                    // This may be possible when reading box is expanded slightly from floating-point roundoff problems.
                    auto values = raster_values[op.values].get();
                    if (values == nullptr) {
                        raster_values[op.values] = op.values->read_box(subgrid.extent().intersection(op.values->grid().extent()));
                        values = raster_values[op.values].get();
                    }

                    if (op.weighted()) {
                        auto weights = raster_values[op.weights].get();
                        if (weights == nullptr) {
                            raster_values[op.weights] = op.weights->read_box(subgrid.extent().intersection(op.weights->grid().extent()));
                            weights = raster_values[op.weights].get();
                        }

                        m_reg.stats(f->first, op).process(*coverage, *values, *weights);
                    } else {
                        m_reg.stats(f->first, op).process(*coverage, *values);
                    }

                    progress();
                }
            }

            progress(subgrid.extent());
        }

        for (const auto& f : m_features) {
            m_output.write(f.first);
            m_reg.flush_feature(f.first);
        }
    }

}
