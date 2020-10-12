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

#include "weighted_quantiles.h"

namespace exactextract {

    void WeightedQuantiles::prepare() const {
        std::sort(m_elems.begin(), m_elems.end(), [](const elem_t &a, const elem_t &b) {
            return a.x < b.x;
        });

        m_sum_w = 0;
        // relies on map being sorted which it is no
        for (size_t i = 0; i < m_elems.size(); i++) {
            m_sum_w += m_elems[i].w;

            if (i == 0) {
                m_elems[i].s = 0;
                m_elems[i].cumsum = m_elems[i].w;
            } else {
                m_elems[i].cumsum = m_elems[i - 1].cumsum + m_elems[i].w;
                m_elems[i].s = i * m_elems[i].w + (static_cast<double>(m_elems.size()) - 1) * m_elems[i - 1].cumsum;
            }
        }

        m_ready_to_query = true;
    }

    double WeightedQuantiles::quantile(double q) const {
        if (!std::isfinite(q) || q < 0 || q > 1) {
            throw std::runtime_error("Quantile must be between 0 and 1.");
        }

        if (!m_ready_to_query) {
            prepare();
        }

        auto sn = m_sum_w * (static_cast<double>(m_elems.size()) - 1);

        elem_t lookup(0, 0); // create a dummy element to use with std::upper_bound
        lookup.s = q*sn;

        // get first element that is greater than or equal to lookup value (q * sn)
        auto right = std::upper_bound(m_elems.cbegin(), m_elems.cend(), lookup, [](const elem_t & a, const elem_t & b) {
            return a.s < b.s;
        });

        if (right == m_elems.cbegin()) {
            return right->x;
        }

        auto left = std::prev(right, 1);

        if (right == m_elems.cend()) {
            return left->x;
        }

        return left->x + (q*sn - left->s)*(right->x - left->x)/(right->s - left->s);
    }

}
