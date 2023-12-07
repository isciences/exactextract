// Copyright (c) 2020 ISciences, LLC.
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

#ifndef EXACTEXTRACT_VARIANCE_H
#define EXACTEXTRACT_VARIANCE_H

#include <cmath>

namespace exactextract {

class WestVariance
{
    /** \brief Implements an incremental algorithm for weighted standard
     * deviation, variance, and coefficient of variation, as described in
     * formula WV2 of West, D.H.D. (1979) "Updating Mean and Variance
     * Estimates: An Improved Method". Communications of the ACM 22(9).
     */

  private:
    double sum_w = 0;
    double mean = 0;
    double t = 0;

  public:
    /** \brief Update variance estimate with another value
     *
     * @param x value to add
     * @param w weight of `x`
     */
    void process(double x, double w)
    {
        if (w == 0) {
            return;
        }

        double mean_old = mean;

        sum_w += w;
        mean += (w / sum_w) * (x - mean_old);
        t += w * (x - mean_old) * (x - mean);
    }

    /** \brief Return the population variance.
     */
    constexpr double variance() const
    {
        return t / sum_w;
    }

    /** \brief Return the population standard deviation
     */
    double stdev() const
    {
        return std::sqrt(variance());
    }

    /** \brief Return the population coefficient of variation
     */
    double coefficent_of_variation() const
    {
        return stdev() / mean;
    }
};

}

#endif // EXACTEXTRACT_VARIANCE_H
