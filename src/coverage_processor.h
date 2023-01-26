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

#ifndef EXACTEXTRACT_RASTER_WEIGHT_PROCESSOR_H
#define EXACTEXTRACT_RASTER_WEIGHT_PROCESSOR_H
/**
 * @file coverage_processor.h
 * @author Nels Frazier (nfrazier@lynker.com)
 * @brief A processor for computing coverage_fraction of raster pixels for vector features
 * @version 0.1
 * @date 2022-03-24
 * 
 * @copyright Copyright (c) 2022
 * 
 * I tried implementing the raster_sequential_processor logic as follows,
 * but I could not get the correct global grid cell number mapping using this
 * so I reverted to simple feature sequential logic, which works well.
 * 
 * void CoverageProcessor::process() {
 *      read_features();
 *      populate_index();
 *      auto grid = m_common_grid;
 *      std::cout<<"HERERERE\n";
 *      for (const auto &subgrid : subdivide(grid, m_max_cells_in_memory)) {
 *            std::vector<const Feature *> hits;
 *
 *            auto query_rect = geos_make_box_polygon(m_geos_context, subgrid.extent());
 *
 *            GEOSSTRtree_query_r(m_geos_context, m_feature_tree.get(), query_rect.get(), [](void *hit, void *userdata) {
 *                auto feature = static_cast<const Feature *>(hit);
 *                auto vec = static_cast<std::vector<const Feature *> *>(userdata);
 *
 *                vec->push_back(feature);
 *            }, &hits);
 *
 *            for (const auto &f : hits) {
 *                std::unique_ptr<Raster<float>> coverage;
 *
 *                // Lazy-initialize coverage
 *                if (coverage == nullptr) {
 *                    coverage = std::make_unique<Raster<float>>(
 *                            raster_cell_intersection(subgrid, m_geos_context, f->second.get()));
 *                }
 *                
 *                m_output.write_coverage(f->first, *coverage, grid);
 *                progress();
 *            }
 *
 *            progress(subgrid.extent());
 *        }
 */

#include "feature_sequential_processor.h"
#include "geos_utils.h"
#include "processor.h"
#include "grid.h"
#include "gdal_raster_wrapper.h"

namespace exactextract {

    class CoverageProcessor : public FeatureSequentialProcessor {
        /*
            Processor dedicated to computing only coverage fraction
        */

    private:
        using RasterMap = std::unordered_map<std::string, GDALRasterWrapper>;
    public:
        
        CoverageProcessor(GDALDatasetWrapper & ds, OutputWriter & out, RasterMap & rasters):
            FeatureSequentialProcessor(ds, out, {}),
            m_common_grid( Grid<bounded_extent>::make_empty() )
        {
            for(const auto& layer : rasters ){
                auto grid = layer.second.grid();
                m_common_grid = grid.common_grid(m_common_grid);
            }
        }
        
        void process() override;

    private:
    
        exactextract::Grid<bounded_extent> m_common_grid;

    };
}

#endif //EXACTEXTRACT_RASTER_WEIGHT_PROCESSOR_H
