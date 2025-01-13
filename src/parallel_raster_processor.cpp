
// Copyright (c) 2024 ISciences, LLC.
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

#include "parallel_raster_processor.h"
#include "operation.h"
#include "raster_cell_intersection.h"
#include "raster_source.h"

#include <cassert>
#include <map>
#include <memory>
#include <ranges>
#include <set>
#include <sstream>

#include <oneapi/tbb.h>

namespace exactextract {

typedef std::vector<const Feature*> FeatureHits;

struct ZonalStatsCalc {
    Grid<bounded_extent> subgrid;
    FeatureHits hits;
    RasterSource* source;
    std::unique_ptr<RasterVariant> values;
};

void
RasterParallelProcessor::read_features()
{
    while (m_shp.next()) {
        const Feature& feature = m_shp.feature();
        MapFeature mf(feature);
        m_features.push_back(std::move(mf));
    }
}

void
RasterParallelProcessor::populate_index()
{
    assert(m_feature_tree != nullptr);

    for (const auto& f : m_features) {
        // TODO compute envelope of dataset, and crop raster by that extent before processing?
        GEOSSTRtree_insert_r(m_geos_context, m_feature_tree.get(), f.geometry(), (void*)&f);
    }
}

void
RasterParallelProcessor::process()
{
    read_features();
    populate_index();

    std::set<RasterSource*> rasterSources;
    for (const auto& op : m_operations) {
        rasterSources.insert(op->values);
    }

    auto grid = common_grid(m_operations.begin(), m_operations.end(), m_grid_compat_tol);
    auto subgrids = subdivide(grid, m_max_cells_in_memory);

    oneapi::tbb::parallel_pipeline(4, 
        oneapi::tbb::make_filter<void, Grid<bounded_extent>>(oneapi::tbb::filter_mode::serial_in_order,
        [&subgrids] (oneapi::tbb::flow_control& fc) -> Grid<bounded_extent> {
            //TODO: split subgridding by raster to optimise IO based on raster block size per input?
            //logic below currently assumes that all raster inputs have the same total geospatial coverage
            if (subgrids.size() == 1) {
                fc.stop();
            }

            auto sg = subgrids.back();
            subgrids.pop_back();
            return sg;
        }) &
        oneapi::tbb::make_filter<Grid<bounded_extent>, std::tuple<Grid<bounded_extent>, FeatureHits>>(oneapi::tbb::filter_mode::parallel,
        [rasterSources, this] (Grid<bounded_extent> subgrid) -> std::tuple<Grid<bounded_extent>, FeatureHits> {
            std::vector<const Feature*> hits;
            auto query_rect = geos_make_box_polygon(m_geos_context, subgrid.extent());

            GEOSSTRtree_query_r(
            m_geos_context, m_feature_tree.get(), query_rect.get(), [](void* hit, void* userdata) {
                auto feature = static_cast<const Feature*>(hit);
                auto vec = static_cast<std::vector<const Feature*>*>(userdata);

                vec->push_back(feature);
            },
            &hits);

            if (hits.empty()) {
                return {Grid<bounded_extent>::make_empty(), FeatureHits()};
            }

            return {subgrid, hits};
        }) &
        oneapi::tbb::make_filter<std::tuple<Grid<bounded_extent>, FeatureHits>, ZonalStatsCalc>(oneapi::tbb::filter_mode::serial_out_of_order,
        [rasterSources, this] (std::tuple<Grid<bounded_extent>, FeatureHits> inputs) -> ZonalStatsCalc {
            auto& subgrid = std::get<0>(inputs);
            auto& hits = std::get<1>(inputs);

            if (rasterSources.size() > 1) {
                    throw std::runtime_error("More than 1 raster sources not yet supported.");
            }
            auto raster = *rasterSources.begin();
            auto values = std::make_unique<RasterVariant>(raster->read_box(subgrid.extent().intersection(raster->grid().extent())));

            return {subgrid, hits, raster, std::move(values)};
        }) &
        oneapi::tbb::make_filter<ZonalStatsCalc, void>(oneapi::tbb::filter_mode::serial_out_of_order,
        [this] (ZonalStatsCalc inputs) {
            if (!inputs.values) {
                throw std::runtime_error("No raster values to process.");
            }

            for (const Feature* f : inputs.hits) {
                auto coverage = std::make_unique<Raster<float>>(raster_cell_intersection(inputs.subgrid, m_geos_context, f->geometry()));
                std::set<std::string> processed;

                for (const auto& op : m_operations) {
                    if (op->values != inputs.source) {
                        continue;
                    }

                    //TODO: push this earlier and remove duplicate operations entirely
                    if (processed.find(op->key()) != processed.end()) {
                        continue;
                    } else {
                        processed.insert(op->key());
                    }

                    if (!op->values->grid().extent().contains(inputs.subgrid.extent())) {
                        continue;
                    }

                    if (op->weighted() && !op->weights->grid().extent().contains(inputs.subgrid.extent())) {
                        continue;
                    }

                    if (op->weighted()) {
                        //TODO: cache weighted input
                        auto weights = std::make_unique<RasterVariant>(op->weights->read_box(inputs.subgrid.extent().intersection(op->weights->grid().extent())));
                        m_reg.update_stats(*f, *op, *coverage, *inputs.values, *weights);
                    } else {
                        m_reg.update_stats(*f, *op, *coverage, *inputs.values);
                    }
                }
            }
        })
    );

    for (const auto& f_in : m_features) {
            write_result(f_in);
    }
}

}
