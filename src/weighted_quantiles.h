// Copyright (c) 2019-2020 ISciences, LLC.
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

#include <cmath>
#include <stdexcept>
#include <vector>

namespace exactextract {

class WeightedQuantiles
{
    // Compute weighted quantiles based on https://stats.stackexchange.com/a/13223

  public:
    void process(double x, double w)
    {
        if (w < 0) {
            throw std::runtime_error("Weighted quantile calculation does not support negative weights.");
        }

        if (!std::isfinite(w)) {
            throw std::runtime_error("Weighted quantile does not support non-finite weights.");
        }

        m_ready_to_query = false;

        m_elems.emplace_back(x, w);
    }

    double quantile(double q) const;

  private:
    struct elem_t
    {
        elem_t(double _x, double _w)
          : x(_x)
          , w(_w)
          , cumsum(0)
          , s(0)
        {
        }

        double x;
        double w;
        double cumsum;
        double s;
    };

    void prepare() const;

    mutable std::vector<elem_t> m_elems;
    mutable double m_sum_w;
    mutable bool m_ready_to_query;
};

}
