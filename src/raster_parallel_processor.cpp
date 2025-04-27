// Copyright (c) 2024-2025 ISciences, LLC.
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

#include "raster_parallel_processor.h"
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

namespace {

void
errorHandlerParallel(const char* fmt, ...)
{

    char buf[BUFSIZ], *p;
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    p = buf + strlen(buf) - 1;
    if (strlen(buf) > 0 && *p == '\n')
        *p = '\0';

    std::cout << "Error: " << buf << std::endl;
}

}

namespace exactextract {

typedef std::vector<const Feature*> FeatureHits;
typedef std::shared_ptr<StatsRegistry> StatsRegistryPtr;

struct ZonalStatsCalc
{
    RasterSource* source = nullptr;
    Grid<bounded_extent> subgrid = Grid<bounded_extent>::make_empty();
    FeatureHits hits = std::vector<const Feature*>();
    std::unique_ptr<RasterVariant> values = nullptr;
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

    std::set<RasterSource*> raster_sources;
    bool raster_read_multithreaded = true;
    for (const auto& op : m_operations) {
        if (op->weighted()) {
            throw std::runtime_error("Weighted operations not yet supported in raster-parallel strategy.");
        } else if (op->requires_variance()) {
            throw std::runtime_error("Variance operations not yet supported in raster-parallel strategy.");
        }

        raster_sources.insert(op->values);
        m_reg.prepare(*op);
    }

    std::vector<ZonalStatsCalc> raster_grids;
    for (auto& source : raster_sources) {
        if (!source->thread_safe()) {
            raster_read_multithreaded = false;
        }

        auto subgrids = subdivide(source->grid(), m_max_cells_in_memory);
        for (auto& subgrid : subgrids) {
            raster_grids.push_back({ .source = source,
                                     .subgrid = subgrid });
        }
    }
    const auto total_subgrids = raster_grids.size();
    size_t processed_subgrids = 0;

    oneapi::tbb::enumerable_thread_specific<GEOSContextHandle_t> geos_context([]() -> GEOSContextHandle_t {
        return initGEOS_r(errorHandlerParallel, errorHandlerParallel);
    });

    if (!raster_read_multithreaded) {
        std::cerr << "exactextract must be built against GDAL 3.10 or later is parallelize raster reads." << std::endl;
        std::cerr << "Computations will be still performed in parallel, however." << std::endl;
    }

    // clang-format off
    oneapi::tbb::parallel_pipeline(m_threads,
        // Fetch an extent to process
        oneapi::tbb::make_filter<void, ZonalStatsCalc>(oneapi::tbb::filter_mode::serial_in_order,
        [&raster_grids] (oneapi::tbb::flow_control& fc) -> ZonalStatsCalc {
            //TODO: split subgridding by raster to optimise IO based on raster block size per input?
            //logic below currently assumes that all raster inputs have the same total geospatial coverage
            if (raster_grids.empty()) {
                fc.stop();
                return {nullptr, Grid<bounded_extent>::make_empty()};
            }

            auto sg = std::move(raster_grids.back());
            raster_grids.pop_back();
            return sg;
        }) &
        // Query for features within this extent
        oneapi::tbb::make_filter<ZonalStatsCalc, ZonalStatsCalc>(oneapi::tbb::filter_mode::parallel,
        [&geos_context, this] (ZonalStatsCalc context) -> ZonalStatsCalc {
            std::vector<const Feature*> hits;
            auto query_rect = geos_make_box_polygon(geos_context.local(), context.subgrid.extent());

            GEOSSTRtree_query_r(
            geos_context.local(), m_feature_tree.get(), query_rect.get(), [](void* hit, void* userdata) {
                auto feature = static_cast<const Feature*>(hit);
                auto vec = static_cast<std::vector<const Feature*>*>(userdata);

                vec->push_back(feature);
            },
            &hits);

            if (hits.empty()) {
                return context;
            }

            context.hits = hits;
            return context;
        }) &
        // Read raster values from this extent
        oneapi::tbb::make_filter<ZonalStatsCalc, ZonalStatsCalc>(raster_read_multithreaded ? oneapi::tbb::filter_mode::parallel : oneapi::tbb::filter_mode::serial_out_of_order,
        [raster_sources, this] (ZonalStatsCalc context) -> ZonalStatsCalc {
            if (context.subgrid.empty() || context.hits.empty()) {
                return context;
            }

            auto values = std::make_unique<RasterVariant>(context.source->read_box(context.subgrid.extent().intersection(context.source->grid().extent())));
            context.values = std::move(values);
            return context;
        }) &
        // Compute coverage fractions and stats for this extent
        oneapi::tbb::make_filter<ZonalStatsCalc, std::tuple<Grid<bounded_extent>, StatsRegistryPtr>>(oneapi::tbb::filter_mode::parallel,
        [&geos_context, this] (ZonalStatsCalc context) -> std::tuple<Grid<bounded_extent>, StatsRegistryPtr> {
            if (context.subgrid.empty() || context.hits.empty() || !context.source || !context.values) {
                return {context.subgrid, nullptr};
            }

            auto block_registry = std::make_shared<StatsRegistry>();
            std::vector<Operation*> trimmed_ops;

            for (const auto& op : m_operations) {
                if (op->values == context.source) {
                    block_registry->prepare(*op);
                    trimmed_ops.push_back(op.get());
                }
            }

            for (const Feature* f : context.hits) {
                auto coverage = std::make_unique<Raster<float>>(raster_cell_intersection(context.subgrid, geos_context.local(), f->geometry()));
                std::set<std::string> processed;

                for (const auto op : trimmed_ops) {
                    //TODO: push this earlier and remove duplicate operations entirely
                    if (processed.find(op->key()) != processed.end()) {
                        continue;
                    } else {
                        processed.insert(op->key());
                    }

                    block_registry->update_stats(*f, *op, *coverage, *context.values);
                }
            }

            return {context.subgrid, block_registry};
        }) &
        // Combine stats for each feature
        oneapi::tbb::make_filter<std::tuple<Grid<bounded_extent>, StatsRegistryPtr>, void>(oneapi::tbb::filter_mode::serial_out_of_order, 
        [&processed_subgrids, total_subgrids, this] (std::tuple<Grid<bounded_extent>, StatsRegistryPtr> registry) {
            auto& [registryGrid, regptr] = registry;
            if (regptr) {
                this->m_reg.merge(*regptr);
            }

            if (m_show_progress && !registryGrid.empty()) {
                processed_subgrids++;

                std::stringstream ss;
                ss << registryGrid.extent();

                progress(static_cast<double>(processed_subgrids) / static_cast<double>(total_subgrids), ss.str());
            }
        })
    );
    // clang-format on

    for (const auto& f_in : m_features) {
        write_result(f_in);
    }
}

}
