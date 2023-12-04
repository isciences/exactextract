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
#include <string>
#include <sstream>

#include "feature.h"
#include "grid.h"
#include "raster_source.h"
#include "raster_stats.h"
#include "stats_registry.h"
#include "raster_coverage_iterator.h"
#include "utils.h"

namespace exactextract {

    /**
     * @brief The Operation class defines a single operation (e.g, "mean", "sum", "coverage")
     *        and the raster(s) to which it is to be applied. When provided with a `StatsRegistry`,
     *        it can look up the result of its operation and assign it to the appropriately named
     *        column(s) in a `Feature`.
     */
    class Operation {
    public:
        Operation(std::string p_stat,
                  std::string p_name,
                  RasterSource* p_values,
                  RasterSource* p_weights = nullptr) :
                stat{std::move(p_stat)},
                name{std::move(p_name)},
                values{p_values},
                weights{p_weights}
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

        virtual std::unique_ptr<Operation> clone() const {
            return std::make_unique<Operation>(*this);
        }

        /// Returns the list of field names that are assigned by this `Operation`
        virtual const std::vector<std::string>& field_names() const {
            return m_field_names;
        }

        /// Returns `true` if the operation uses values from a weighting raster
        bool weighted() const {
            return weights != nullptr;
        }

        /// Returns a newly constructed `Grid` representing the common grid between
        /// the value and weighting rasters
        Grid<bounded_extent> grid() const {
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
        virtual const std::string& key() const {
            return m_key;
        }

        void set_quantiles(const std::vector<double>& quantiles) {
            m_quantiles = quantiles;
            setQuantileFieldNames();
        }


        virtual void set_result(const StatsRegistry& reg, const Feature& f_in, Feature& f_out) const {
            static const RasterStats<double> empty_stats;

            const RasterStats<double>& stats = reg.contains(f_in, *this) ? reg.stats(f_in, *this) : empty_stats;

            auto missing = std::numeric_limits<double>::quiet_NaN();

            if (stat == "mean") {
                f_out.set(m_field_names[0], stats.mean());
            } else if (stat == "sum") {
                f_out.set(m_field_names[0], stats.sum());
            } else if (stat == "count") {
                f_out.set(m_field_names[0], stats.count());
            } else if (stat == "weighted_mean") {
                f_out.set(m_field_names[0], stats.weighted_mean());
            } else if (stat == "weighted_sum") {
                f_out.set(m_field_names[0], stats.weighted_sum());
            } else if (stat == "min") {
                f_out.set(m_field_names[0], stats.min().value_or(missing));
            } else if (stat == "max") {
                f_out.set(m_field_names[0], stats.max().value_or(missing));
            } else if (stat == "majority" || stat == "mode") {
                f_out.set(m_field_names[0], stats.mode().value_or(missing));
            } else if (stat == "minority") {
                f_out.set(m_field_names[0], stats.minority().value_or(missing));
            } else if (stat == "variety") {
                f_out.set(m_field_names[0], stats.variety());
            } else if (stat == "stdev") {
                f_out.set(m_field_names[0], stats.stdev());
            } else if (stat == "weighted_stdev") {
                f_out.set(m_field_names[0], stats.weighted_stdev());
            } else if (stat == "variance") {
                f_out.set(m_field_names[0], stats.variance());
            } else if (stat == "weighted_variance") {
                f_out.set(m_field_names[0], stats.weighted_variance());
            } else if (stat == "coefficient_of_variation") {
                f_out.set(m_field_names[0], stats.coefficient_of_variation());
            } else if (stat == "median") {
                f_out.set(m_field_names[0], stats.quantile(0.5).value_or(std::numeric_limits<double>::quiet_NaN()));
            } else if (stat == "quantile") {
                for (std::size_t i = 0; i < m_quantiles.size(); i++) {
                    f_out.set(m_field_names[i], stats.quantile(m_quantiles[i]).value_or(std::numeric_limits<double>::quiet_NaN()));
                }
            } else if (stat == "frac") {
                for (const auto& value : stats) {
                    std::stringstream s;
                    s << "frac_" << value;

                    f_out.set(s.str(), stats.frac(value).value_or(0));
                }
            } else if (stat == "weighted_frac") {
                for (const auto& value : stats) {
                    std::stringstream s;
                    s << "weighted_frac_" << value;

                    f_out.set(s.str(), stats.weighted_frac(value).value_or(0));
                }
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

#endif //EXACTEXTRACT_OPERATION_H
