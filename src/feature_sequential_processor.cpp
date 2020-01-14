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

#include <set>
#include <string>

#include "box.h"
#include "feature_sequential_processor.h"
#include "geos_utils.h"
#include "grid.h"

namespace exactextract {

    void FeatureSequentialProcessor::process() {
        for (const auto& op : m_operations) {
            m_output.add_operation(op);
        }

        while (m_shp.next()) {
            std::string name{m_shp.feature_field(m_shp.id_field())};
            auto geom = geos_ptr(m_geos_context, m_shp.feature_geometry(m_geos_context));

            progress(name);

            Box feature_bbox = exactextract::geos_get_box(m_geos_context, geom.get());

            auto grid = common_grid(m_operations.begin(), m_operations.end());

            if (feature_bbox.intersects(grid.extent())) {
                // Crop grid to portion overlapping feature
                auto cropped_grid = grid.crop(feature_bbox);

                for (const auto &subgrid : subdivide(cropped_grid, m_max_cells_in_memory)) {
                    std::unique_ptr<Raster<float>> coverage;

                    std::set<std::pair<RasterSource*, RasterSource*>> processed;

                    for (const auto &op : m_operations) {
                        // TODO avoid reading same values/weights multiple times. Just use a map?

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
                                    raster_cell_intersection(subgrid, m_geos_context, geom.get()));
                        }

                        auto values = op.values->read_box(subgrid.extent().intersection(op.values->grid().extent()));

                        if (op.weighted()) {
                            auto weights = op.weights->read_box(subgrid.extent().intersection(op.weights->grid().extent()));

                            m_reg.stats(name, op).process(*coverage, *values, *weights);
                        } else {
                            m_reg.stats(name, op).process(*coverage, *values);
                        }

                        progress();
                    }
                }
            }

            m_output.write(name);
            m_reg.flush_feature(name);
        }
    }
}
