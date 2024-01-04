// Copyright (c) 2019-2024 ISciences, LLC.
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

#include "stats_registry.h"

#include "operation.h"
#include "raster_stats.h"

namespace exactextract {

void
StatsRegistry::prepare(const std::string& stat)
{
    m_stats_options.store_histogram |= requires_stored_values(stat);
    m_stats_options.store_values |= (stat == "values");
    m_stats_options.store_weights |= (stat == "weights");
    m_stats_options.store_coverage_fraction |= (stat == "coverage");
    m_stats_options.store_x |= (stat == "center_x" || stat == "cell_id");
    m_stats_options.store_y |= (stat == "center_y" || stat == "cell_id");
}

void
StatsRegistry::update_stats(const Feature& f, const Operation& op, const Raster<float>& coverage, const RasterVariant& values)
{
    std::visit([&coverage, &values](auto& s) {
        using value_type = typename std::remove_reference_t<decltype(s)>::ValueType;

        const AbstractRaster<value_type>& v = *std::get<std::unique_ptr<AbstractRaster<value_type>>>(
          values);

        s.process(coverage, v);
    },
               stats(f, op));
}

void
StatsRegistry::update_stats(const Feature& f, const Operation& op, const Raster<float>& coverage, const RasterVariant& values, const RasterVariant& weights)
{
    std::visit([&coverage, &values](auto& s, const auto& w) {
        using value_type = typename std::remove_reference_t<decltype(s)>::ValueType;

        const AbstractRaster<value_type>& v = *std::get<std::unique_ptr<AbstractRaster<value_type>>>(
          values);

        s.process(coverage, v, *w);
    },
               stats(f, op),
               weights);
}

StatsRegistry::RasterStatsVariant&
StatsRegistry::stats(const Feature& feature, const Operation& op)
{
    auto& stats_for_feature = m_feature_stats[&feature];

    auto it = stats_for_feature.find(op.key());
    if (it == stats_for_feature.end()) {
        // Construct a RasterStats with the correct value type for this Operation.
        const auto& rast = op.values->read_empty();

        it = std::visit([&stats_for_feature, &op, this](const auto& r) {
                 using value_type = typename std::remove_reference_t<decltype(*r)>::value_type;

                 return stats_for_feature.emplace(op.key(), RasterStats<value_type>(m_stats_options));
             },
                        rast)
               .first;
    }

    return it->second;
}

bool
StatsRegistry::contains(const Feature& feature, const Operation& op) const
{
    const auto& m = m_feature_stats;

    auto it = m.find(&feature);

    if (it == m.end()) {
        return false;
    }

    const auto& m2 = it->second;

    return m2.find(op.key()) != m2.end();
}

const StatsRegistry::RasterStatsVariant&
StatsRegistry::stats(const Feature& feature, const Operation& op) const
{
    return m_feature_stats.at(&feature).at(op.key());
}
}
