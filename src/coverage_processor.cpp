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

/**
 * @file coverage_processor.cpp
 * @author Nels Frazier (nfrazier@lynker.com)
 * @brief Implementation of coverage processor
 * @version 0.1
 * @date 2022-03-24
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "coverage_processor.h"

#include <map>
#include <memory>
#include <set>

namespace exactextract {

    void CoverageProcessor::process() {
    
        while (m_shp.next()) {
            std::string name{ m_shp.feature_field(m_shp.id_field()) };
            auto geom = geos_ptr(m_geos_context, m_shp.feature_geometry(m_geos_context));

            progress(name);

            Box feature_bbox = exactextract::geos_get_box(m_geos_context, geom.get());

            auto grid = m_common_grid; 
            if (feature_bbox.intersects(grid.extent())) {
                // Crop grid to portion overlapping feature
                auto cropped_grid = grid.crop(feature_bbox);

                for (const auto &subgrid : subdivide(cropped_grid, m_max_cells_in_memory)) {
                    std::unique_ptr<Raster<float>> coverage;

                    // Lazy-initialize coverage
                    if (coverage == nullptr) {
                        coverage = std::make_unique<Raster<float>>(
                                raster_cell_intersection(subgrid, m_geos_context, geom.get()));
                    }

                    m_output.write_coverage(name, *coverage, grid);

                    progress();
                    
                }
            }

        }

    }

}
