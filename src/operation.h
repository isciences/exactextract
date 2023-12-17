// Copyright (c) 2019-2023 ISciences, LLC.
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

#ifndef EXACTEXTRACT_OPERATION_H
#define EXACTEXTRACT_OPERATION_H

#include <functional>
#include <sstream>
#include <string>

#include "feature.h"
#include "grid.h"
#include "raster_coverage_iterator.h"
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
    virtual const std::vector<std::string>& field_names() const
    {
        return m_field_names;
    }

    /// Returns `true` if the operation uses values from a weighting raster
    bool weighted() const
    {
        return weights != nullptr;
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

    virtual void set_result(const StatsRegistry& reg, const Feature& f_in, Feature& f_out) const
    {
        static const StatsRegistry::RasterStatsVariant empty_stats = RasterStats<double>();

        constexpr bool write_if_missing = true; // should we set attribute values if the feature did not intersect the raster?
        if (!write_if_missing && !reg.contains(f_in, *this)) {
            return;
        }

        const auto& stats = reg.contains(f_in, *this) ? reg.stats(f_in, *this) : empty_stats;

        // FIXME don't read an empty box every time, maybe cache this in the source?
        auto empty_rast = values->read_box(Box::make_empty());

        auto missing = std::visit([](const auto& r) {
            std::variant<std::int32_t, std::int64_t, float, double> ret = std::numeric_limits<double>::quiet_NaN();
            if (r->has_nodata()) {
                ret = r->nodata();
            }
            return ret;
        },
                                  empty_rast);

        if (stat == "mean") {
            std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.mean()); }, stats);
        } else if (stat == "sum") {
            std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.sum()); }, stats);
        } else if (stat == "count") {
            std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.count()); }, stats);
        } else if (stat == "weighted_mean") {
            std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.weighted_mean()); }, stats);
        } else if (stat == "weighted_sum") {
            std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.weighted_sum()); }, stats);
        } else if (stat == "min") {
            std::visit([&f_out, this](const auto& x, const auto& m) { f_out.set(m_field_names[0], x.min().value_or(m)); }, stats, missing);
        } else if (stat == "max") {
            std::visit([&f_out, this](const auto& x, const auto& m) { f_out.set(m_field_names[0], x.max().value_or(m)); }, stats, missing);
        } else if (stat == "majority" || stat == "mode") {
            std::visit([&f_out, this](const auto& x, const auto& m) { f_out.set(m_field_names[0], x.mode().value_or(m)); }, stats, missing);
        } else if (stat == "minority") {
            std::visit([&f_out, this](const auto& x, const auto& m) { f_out.set(m_field_names[0], x.minority().value_or(m)); }, stats, missing);
        } else if (stat == "variety") {
            std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.variety()); }, stats);
        } else if (stat == "stdev") {
            std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.stdev()); }, stats);
        } else if (stat == "weighted_stdev") {
            std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.weighted_stdev()); }, stats);
        } else if (stat == "variance") {
            std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.variance()); }, stats);
        } else if (stat == "weighted_variance") {
            std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.weighted_variance()); }, stats);
        } else if (stat == "coefficient_of_variation") {
            std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.coefficient_of_variation()); }, stats);
        } else if (stat == "median") {
            std::visit([&f_out, this](const auto& x, const auto& m) { f_out.set(m_field_names[0], x.quantile(0.5).value_or(m)); }, stats, missing);
        } else if (stat == "quantile") {
            std::visit([&f_out, this](const auto& x, const auto& m) {
                for (std::size_t i = 0; i < m_quantiles.size(); i++) {
                    f_out.set(m_field_names[i], x.quantile(m_quantiles[i]).value_or(m));
                }
            },
                       stats,
                       missing);
        } else if (stat == "frac") {
            std::visit([&f_out](const auto& x) {
                for (const auto& value : x) {
                    std::stringstream s;
                    s << "frac_" << value;

                    f_out.set(s.str(), x.frac(value).value_or(0));
                }
            },
                       stats);
        } else if (stat == "weighted_frac") {
            std::visit([&f_out](const auto& x) {
                for (const auto& value : x) {
                    std::stringstream s;
                    s << "weighted_frac_" << value;

                    f_out.set(s.str(), x.weighted_frac(value).value_or(0));
                }
            },
                       stats);
        } else {
            throw std::runtime_error("Unhandled stat: " + stat);
        }
    }

    std::string stat;
    std::string name;
    RasterSource* values;
    RasterSource* weights;

  private:
    std::string m_key;

    void setQuantileFieldNames();

    std::vector<double> m_quantiles;

  protected:
    std::vector<std::string> m_field_names;
};

}

#endif // EXACTEXTRACT_OPERATION_H
