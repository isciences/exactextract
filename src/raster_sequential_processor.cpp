// Copyright (c) 2019 ISciences, LLC.
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

#include <set>

namespace exactextract {

    void RasterSequentialProcessor::read_features() {
        while (m_shp.next()) {
            Feature feature = std::make_pair(
                    m_shp.feature_field(m_field_name),
                    geos_ptr(m_geos_context, m_shp.feature_geometry(m_geos_context)));
            features.push_back(std::move(feature));
        }
    }

    void RasterSequentialProcessor::populate_index() {
        for (const Feature& f : features) {
            // TODO compute envelope of dataset, and crop raster by that extent before processing?
            GEOSSTRtree_insert_r(m_geos_context, feature_tree.get(), f.second.get(), (void *) &f);
        }
    }

    void RasterSequentialProcessor::process() {
        read_features();
        populate_index();

        auto grid = common_grid(m_operations.begin(), m_operations.end());

        for (const auto &subgrid : subdivide(grid, m_max_cells_in_memory)) {
            std::set<std::pair<GDALRasterWrapper*, GDALRasterWrapper*>> processed;
            std::vector<const Feature *> hits;

            auto query_rect = geos_make_box_polygon(m_geos_context, subgrid.extent());

            GEOSSTRtree_query_r(m_geos_context, feature_tree.get(), query_rect.get(), [](void *hit, void *userdata) {
                auto feature = static_cast<const Feature *>(hit);
                auto vec = static_cast<std::vector<const Feature *> *>(userdata);

                vec->push_back(feature);
            }, &hits);

            for (const auto &f : hits) {
                // TODO avoid reading same values, weights multiple times. Just use a map?

                std::unique_ptr<Raster<float>> coverage;

                for (const auto &op : m_operations) {
                    // Avoid processing same values/weights for different stats
                    if (processed.find(std::make_pair(op.weights, op.values)) != processed.end())
                        continue;

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

                    auto op_subgrid = subgrid.overlapping_grid(op.values->grid());

                    if (op.weighted()) {
                        op_subgrid = op_subgrid.overlapping_grid(op.weights->grid());
                    }

                    Raster<double> values = op.values->read_box(op_subgrid.extent());

                    if (op.weighted()) {
                        Raster<double> weights = op.weights->read_box(op_subgrid.extent());
                    }
                }
            }

            progress(subgrid.extent());
        }
    }

}
