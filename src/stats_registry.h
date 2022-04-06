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

#ifndef EXACTEXTRACT_STATS_REGISTRY_H
#define EXACTEXTRACT_STATS_REGISTRY_H

#include <string>
#include <unordered_map>

#include "operation.h"
#include "raster_stats.h"

namespace exactextract {

    class StatsRegistry {
    public:
        RasterStats<double> &stats(const std::string &feature, const Operation &op) {
            // TODO come up with a better storage method.
            auto& stats_for_feature = m_feature_stats[feature];

            // can't use find because this requires RasterStats to be copy-constructible before C++ 17
            auto exists = stats_for_feature.count(op_key(op));
            if (!exists) {
                // can't use emplace because this requires RasterStats be copy-constructible before C++17
                RasterStats<double> new_stats(requires_stored_values(op.stat));
                stats_for_feature[op_key(op)] = std::move(new_stats);
            }

            return stats_for_feature[op_key(op)];
        }

        const RasterStats<double> &stats(const std::string &feature, const Operation &op) const {
            // TODO come up with a better storage method.
            return m_feature_stats.at(feature).at(op_key(op));
        }

        bool contains (const std::string & feature, const Operation & op) const {
            const auto& m = m_feature_stats;

            auto it = m.find(feature);

            if (it == m.end()) {
                return false;
            }

            const auto& m2 = it->second;

            return m2.find(op_key(op)) != m2.end();
        }

        void flush_feature(const std::string &fid) {
            std::unordered_map<std::string, double> vals;
            // TODO assemble vals;

            m_feature_stats.erase(fid);
        }

        std::string op_key(const Operation & op) const {
            if (op.weighted()) {
                return op.values->name() + "|" + op.weights->name();
            } else {
                return op.values->name();
            }
        }

        static bool requires_stored_values(const std::string & stat) {
            return stat == "mode" || stat == "minority" || stat == "majority" || stat == "variety";
        }


    private:
        std::unordered_map<std::string,
        std::unordered_map<std::string, RasterStats <double>>> m_feature_stats{};
    };

}

#endif //EXACTEXTRACT_STATS_REGISTRY_H
