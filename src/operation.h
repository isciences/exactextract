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

#pragma once

#include <string>

#include "feature.h"
#include "grid.h"
#include "raster_source.h"
#include "raster_stats.h"
#include "stats_registry.h"
#include "utils.h"

namespace exactextract {
/**
 * @brief The Operation class defines a single operation (e.g, "mean", "sum", "coverage")
 *        and the raster(s) to which it is to be applied. When provided with a `StatsRegistry`,
 *        it can look up the result of its operation and assign it to the appropriately named
 *        column(s) in a `Feature`.
 */
class Operation
{
  public:
    Operation(std::string p_stat,
              std::string p_name,
              RasterSource* p_values,
              RasterSource* p_weights = nullptr)
      : stat{ std::move(p_stat) }
      , name{ std::move(p_name) }
      , values{ p_values }
      , weights{ p_weights }
      , m_missing{ get_missing_value() }
    {
        if (stat == "quantile") {
            setQuantileFieldNames();
        } else {
            m_field_names.push_back(name);
        }

        if (starts_with(stat, "weighted") && weights == nullptr) {
            throw std::runtime_error("No weights provided for weighted stat: " + stat);
        }

        if (weighted()) {
            m_key = values->name() + "|" + weights->name();
        } else {
            m_key = values->name();
        }
    }

    virtual std::unique_ptr<Operation> clone() const
    {
        return std::make_unique<Operation>(*this);
    }

    /// Returns the list of field names that are assigned by this `Operation`
    const std::vector<std::string>& field_names() const
    {
        return m_field_names;
    }

    /// Returns `true` if the rasters associated with this Operation intersect the box
    bool intersects(const Box& box) const
    {
        if (!values->grid().extent().intersects(box)) {
            return false;
        }

        if (weighted() && !weights->grid().extent().intersects(box)) {
            return false;
        }

        return true;
    }

    /// Returns `true` if the operation uses values from a weighting raster
    bool weighted() const
    {
        return weights != nullptr;
    }

    bool column_names_known() const
    {
        return !(stat == "frac" || stat == "weighted_frac");
    }

    bool requires_histogram() const
    {
        return stat == "mode" || stat == "minority" || stat == "majority" || stat == "variety" || stat == "quantile" || stat == "median" || stat == "frac" || stat == "weighted_frac";
    }

    bool requires_stored_values() const
    {
        return stat == "values";
    }

    bool requires_stored_weights() const
    {
        return stat == "weights";
    }

    bool requires_stored_coverage_fractions() const
    {
        return stat == "coverage";
    }

    bool requries_stored_locations() const
    {
        return stat == "center_x" || stat == "center_y" || stat == "cell_id" || stat == "min_center_x" || stat == "min_center_y" || stat == "max_center_x" || stat == "max_center_y";
    }

    /// Returns a newly constructed `Grid` representing the common grid between
    /// the value and weighting rasters
    Grid<bounded_extent> grid() const
    {
        if (weighted()) {
            return values->grid().common_grid(weights->grid());
        } else {
            return values->grid();
        }
    }

    /// Returns a key for which a `StatsRegistry` can be queried to get a
    /// `RasterStats` object which can be used to populate the columns associated
    /// with this `Operation. Does _not_ need to be unique to the
    /// `Operation`, as multple Operations may be derived from the same
    /// `RasterStats`.
    virtual const std::string& key() const
    {
        return m_key;
    }

    void set_quantiles(const std::vector<double>& quantiles)
    {
        m_quantiles = quantiles;
        setQuantileFieldNames();
    }

    void set_result(const StatsRegistry& reg, const Feature& f_in, Feature& f_out) const;

    void set_result(const StatsRegistry::RasterStatsVariant& stats, Feature& f_out) const;

    void set_empty_result(Feature& f_out) const;

    std::string stat;
    std::string name;
    RasterSource* values;
    RasterSource* weights;

  private:
    std::string m_key;

    void setQuantileFieldNames();

    std::vector<double> m_quantiles;

    using missing_value_t = std::variant<double, std::int8_t, std::int16_t, std::int32_t, std::int64_t>;

    missing_value_t m_missing;

    missing_value_t get_missing_value();

    const StatsRegistry::RasterStatsVariant& empty_stats() const;

  protected:
    std::vector<std::string> m_field_names;
};
}
