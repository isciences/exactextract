// Copyright (c) 2023 ISciences, LLC.
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

#include "coverage_processor.h"
#include "coverage_operation.h"
#include "raster_area.h"
#include "raster_coverage_iterator.h"

namespace exactextract {

void
CoverageProcessor::process()
{
    if (m_operations.empty()) {
        return;
    }

    if (m_operations.size() > 1) {
        throw std::runtime_error("Unexpected operation in CoverageProcessor.");
    }

    if (dynamic_cast<CoverageOperation*>(m_operations.front().get()) == nullptr) {
        throw std::runtime_error("Only CoverageOperation can be handled by CoverageProcessor.");
    }

    CoverageOperation& op = static_cast<CoverageOperation&>(*m_operations.front());
    m_output.add_operation(op);

    const auto& opts = op.options();

    auto grid = common_grid(m_operations.begin(), m_operations.end());

    RasterSource* values = opts.include_values ? op.values : nullptr;
    RasterSource* weights = opts.include_weights ? op.weights : nullptr;

    RasterVariant vx = values ? values->read_box(Box::make_empty()) : std::make_unique<Raster<double>>(Raster<double>::make_empty());
    RasterVariant wx = weights ? weights->read_box(Box::make_empty()) : std::make_unique<Raster<double>>(Raster<double>::make_empty());

    while (m_shp.next()) {
        const Feature& f_in = m_shp.feature();

        progress(f_in, m_include_cols.empty() ? "." : m_include_cols.front());

        auto geom = f_in.geometry();
        auto feature_bbox = geos_get_box(m_geos_context, geom);

        // Crop grid to portion overlapping feature
        auto cropped_grid = grid.crop(feature_bbox);

        for (const auto& subgrid : subdivide(cropped_grid, m_max_cells_in_memory)) {
            auto coverage_fractions = raster_cell_intersection(subgrid, m_geos_context, geom);

            std::unique_ptr<AbstractRaster<double>> areas;
            if (opts.area_method == CoverageOperation::AreaMethod::CARTESIAN) {
                areas = std::make_unique<CartesianAreaRaster<double>>(coverage_fractions.grid());
            } else if (opts.area_method == CoverageOperation::AreaMethod::SPHERICAL) {
                areas = std::make_unique<SphericalAreaRaster<double>>(coverage_fractions.grid());
            }

            std::visit([this, &op, &f_in, &coverage_fractions, &values, &weights, &grid, &areas](const auto& _v, const auto& _w) {
                using ValueType = typename std::remove_reference_t<decltype(*_v)>::value_type;
                using WeightType = typename std::remove_reference_t<decltype(*_w)>::value_type;

                for (const auto& loc : RasterCoverageIteration<ValueType, WeightType>(coverage_fractions, values, weights, grid, areas.get())) {

                    auto f_out = m_output.create_feature();
                    for (const auto& col : m_include_cols) {
                        f_out->set(col, f_in);
                    }

                    (void)loc;

                    op.set_coverage_result(loc, *f_out);
                    m_output.write(*f_out);
                }
            },
                       vx,
                       wx);

            progress();
        }
    }
}

}
