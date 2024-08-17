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

#include <map>
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
    using ArgMap = std::map<std::string, std::string>;

    /// \param stat The name of the statistic computed by this Operation
    /// \param name The base of the field name to which the result of the Operation will be written.
    /// \param values The RasterSource from which values will be read
    /// \param weights The RasterSource from which weights will be read, if `stat` is a weighted
    ///                stat.
    /// \param options Optional arguments to be processed by this stat
    static std::unique_ptr<Operation> create(std::string stat,
                                             std::string name,
                                             RasterSource* values,
                                             RasterSource* weights = nullptr,
                                             ArgMap options = {});

    virtual ~Operation() = default;

    virtual std::unique_ptr<Operation> clone() const = 0;

    /// Returns `true` if the rasters associated with this Operation intersect the box
    bool intersects(const Box& box) const;

    /// Returns `true` if the operation uses values from a weighting raster
    bool weighted() const
    {
        return weights != nullptr;
    }

    /// Method which an implementation can override to indicate that variance or
    /// standard deviation values will be read from a `RasterStats`.
    virtual bool requires_variance() const
    {
        return false;
    }

    /// Method which an implementation can override to indicate that `RasterStats`
    /// should prepare a histogram.
    virtual bool requires_histogram() const
    {
        return false;
    }

    /// Method which an implementation can override to indicate that `RasterStats`
    /// should retain the value for every cell it processes.
    virtual bool requires_stored_values() const
    {
        return false;
    }

    /// Method which an implementation can override to indicate that `RasterStats`
    /// should retain the weight for every cell it processes.
    virtual bool requires_stored_weights() const
    {
        return false;
    }

    /// Method which an implementation can override to indicate that `RasterStats`
    /// should retain the coverage fraction for every cell it processes.
    virtual bool requires_stored_coverage_fractions() const
    {
        return false;
    }

    /// Method which an implementation can override to indicate that `RasterStats`
    /// should retain the center coordinates for every cell it processes.
    virtual bool requires_stored_locations() const
    {
        return false;
    }

    /// Return the minimum fraction of a pixel that must be covered for its value
    /// to be considered in this `Operation`.
    float min_coverage() const
    {
        return m_min_coverage;
    }

    template<typename T>
    std::optional<T> default_value() const
    {
        if (!m_default_value.has_value()) {
            return std::nullopt;
        }

        return { string::read<T>(m_default_value.value()) };
    }

    std::optional<double> default_weight() const
    {
        return m_default_weight;
    }

    /// Returns the method by which pixel coverage should be considered in this `Operation`
    CoverageWeightType coverage_weight_type() const
    {
        return m_coverage_weight_type;
    }

    /// Returns a newly constructed `Grid` representing the common grid between
    /// the value and weighting rasters
    Grid<bounded_extent> grid(double ncompat_tol) const;

    /// Returns the type of the `Operation` result
    virtual Feature::ValueType result_type() const = 0;

    /// Returns a key for which a `StatsRegistry` can be queried to get a
    /// `RasterStats` object which can be used to populate the columns associated
    /// with this `Operation. Does _not_ need to be unique to the
    /// `Operation`, as multple Operations may be derived from the same
    /// `RasterStats`.
    virtual const std::string& key() const
    {
        return m_key;
    }

    /// Assigns the result of the `Operation` to `f_out`, given a `StatsRegistry` can a Feature to query it with.
    void set_result(const StatsRegistry& reg, const Feature& f_in, Feature& f_out) const;

    /// Assigns the result of the `Operation` to `f_out`, given a `RasterStats` from which to read values.
    virtual void set_result(const StatsRegistry::RasterStatsVariant& stats, Feature& f_out) const = 0;

    /// Populates a field in `f_out` with a missing value
    void set_empty_result(Feature& f_out) const;

    const ArgMap& options() const
    {
        return m_options;
    }

    StatDescriptor descriptor() const;

    std::string stat;
    std::string name;
    RasterSource* values;
    RasterSource* weights;

  protected:
    Operation(std::string stat,
              std::string name,
              RasterSource* values,
              RasterSource* weights,
              ArgMap& options);

    std::string m_key;

    using missing_value_t = std::variant<float, double, std::int8_t, std::uint8_t, std::int16_t, std::uint16_t, std::int32_t, std::uint32_t, std::int64_t, std::uint64_t>;

    missing_value_t m_missing;

    std::optional<std::string> m_default_value;
    std::optional<double> m_default_weight;

    missing_value_t get_missing_value();

    const StatsRegistry::RasterStatsVariant& empty_stats() const;

    float m_min_coverage;
    CoverageWeightType m_coverage_weight_type;

  private:
    const ArgMap m_options;
};

}
