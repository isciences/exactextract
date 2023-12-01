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

#ifndef EXACTEXTRACT_STATS_REGISTRY_H
#define EXACTEXTRACT_STATS_REGISTRY_H

#include <string>
#include <unordered_map>
#include <set>

#include "raster_stats.h"

namespace exactextract {
    class Operation;
}

namespace exactextract {

    /**
     * @brief The StatsRegistry class stores an instance of a `RasterStats` object that can be associated
     * with a feature and one or more Operations sharing a the same key.
     */
    class StatsRegistry {
    public:
        /**
         * @brief Get the RasterStats object for a given feature id/operation, creating it if necessary.
         */
        RasterStats<double> &stats(const std::string &feature, const Operation &op, bool store_values);

        const RasterStats<double>& stats(const std::string &feature, const Operation &op) const;

        /**
         * @brief Determine if a `RasterStats` object exists for a given feature id/operation
         */
        bool contains(const std::string & feature, const Operation & op) const;

        /**
         * @brief Remove RasterStats objects associated with a given feature id
         */
        void flush_feature(const std::string &fid) {
            m_feature_stats.erase(fid);
        }

        static bool requires_stored_values(const std::string & stat) {
            return stat == "mode" || stat == "minority" || stat == "majority" || stat == "variety" || stat == "quantile" || stat == "median" || stat == "frac" || stat == "weighted_frac";
        }

        template<typename T>
        static bool requires_stored_values(const T& ops) {
            return std::any_of(ops.begin(),
                           ops.end(),
                           [](const auto& op) {
                                return requires_stored_values(op->stat);
            });
        }

    private:

        std::unordered_map<std::string,
        std::unordered_map<std::string, RasterStats <double>>> m_feature_stats{};
    };

}

#endif //EXACTEXTRACT_STATS_REGISTRY_H
