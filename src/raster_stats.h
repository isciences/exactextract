// Copyright (c) 2018 ISciences, LLC.
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

#ifndef EXACTEXTRACT_RASTER_STATS_H
#define EXACTEXTRACT_RASTER_STATS_H

#include <algorithm>
#include <limits>
#include <unordered_map>

#include "raster_cell_intersection.h"

namespace exactextract {

    template<typename T>
    class RasterStats {

    public:
        /**
         * Compute raster statistics from a Raster representing intersection percentages,
         * a Raster representing data values, and (optionally) a Raster representing weights.
         * and a set of raster values.
         */
        explicit RasterStats(bool store_values = true) :
                m_min{std::numeric_limits<T>::max()},
                m_max{std::numeric_limits<T>::lowest()},
                m_sum_ciwi{0},
                m_sum_ci{0},
                m_sum_xici{0},
                m_sum_xiciwi{0},
                m_store_values{store_values} {}

        void process(const Raster<float> & intersection_percentages, const AbstractRaster<T> & rast) {
            RasterView<T> rv{rast, intersection_percentages.grid()};

            for (size_t i = 0; i < rv.rows(); i++) {
                for (size_t j = 0; j < rv.cols(); j++) {
                    float pct_cov = intersection_percentages(i, j);
                    T val;
                    if (pct_cov > 0 && rv.get(i, j, val)) {
                        process_value(val, pct_cov, 1.0);
                    }
                }
            }
        }

        void process(const Raster<float> & intersection_percentages, const AbstractRaster<T> & rast, const AbstractRaster<T> & weights) {
            auto common = rast.grid().overlapping_grid(weights.grid());
            common = common.overlapping_grid(intersection_percentages.grid());

            if (common.empty())
                return;

            RasterView<float> iv{intersection_percentages, common};
            RasterView<T> rv{rast,    common};
            RasterView<T> wv{weights, common};

            for (size_t i = 0; i < rv.rows(); i++) {
                for (size_t j = 0; j < rv.cols(); j++) {
                    float pct_cov = iv(i, j);
                    T weight;
                    T val;

                    if (pct_cov > 0 && wv.get(i, j, weight) && rv.get(i, j, val)) {
                        process_value(val, pct_cov, weight);
                    }
                }
            }
        }

        /**
         * The mean value of cells covered by this polygon, weighted
         * by the percent of the cell that is covered.
         */
        float mean() const {
            return sum() / count();
        }

        /**
         * The mean value of cells covered by this polygon, weighted
         * by the percent of the cell that is covered and a secondary
         * weighting raster.
         */
         float weighted_mean() const {
             return weighted_sum() / weighted_count();
         }

         /** The fraction of weighted cells to unweighted cells.
          *  Meaningful only when the values of the weighting
          *  raster are between 0 and 1.
          */
         float weighted_fraction() const {
             return weighted_sum() / sum();
         }

        /**
         * The raster value occupying the greatest number of cells
         * or partial cells within the polygon. When multiple values
         * cover the same number of cells, the greatest value will
         * be returned. Weights are not taken into account.
         */
        T mode() const {
            return std::max_element(m_freq.cbegin(),
                                    m_freq.cend(),
                                    [](const auto &a, const auto &b) {
                                        return a.second < b.second || (a.second == b.second && a.first < b.first);
                                    })->first;
        }

        /**
         * The minimum value in any raster cell wholly or partially covered
         * by the polygon. Weights are not taken into account.
         */
        T min() const {
            return m_min;
        }

        /**
         * The maximum value in any raster cell wholly or partially covered
         * by the polygon. Weights are not taken into account.
         */
        T max() const {
            return m_max;
        }

        /**
         * The sum of raster cells covered by the polygon, with each raster
         * value weighted by its coverage fraction.
         */
        float sum() const {
            return (float) m_sum_xici;
        }

        /**
         * The sum of raster cells covered by the polygon, with each raster
         * value weighted by its coverage fraction and weighting raster value.
         */
        float weighted_sum() const {
            return (float) m_sum_xiciwi;
        }

        /**
         * The number of raster cells with a defined value
         * covered by the polygon. Weights are not taken
         * into account.
         */
        float count() const {
            return (float) m_sum_ci;
        }

        /** The sum of weights for each cell covered by the
         *  polygon, with each weight multiplied by the coverage
         *  coverage fraction of each cell.
         */
        float weighted_count() const {
            return (float) m_sum_ciwi;
        }

        /**
         * The raster value occupying the least number of cells
         * or partial cells within the polygon. When multiple values
         * cover the same number of cells, the lowest value will
         * be returned.
         *
         * Cell weights are not taken into account.
         */
        T minority() const {
            return std::min_element(m_freq.cbegin(),
                                    m_freq.cend(),
                                    [](const auto &a, const auto &b) {
                                        return a.second < b.second || (a.second == b.second && a.first < b.first);
                                    })->first;
        }

        /**
         * The number of distinct defined raster values in cells wholly
         * or partially covered by the polygon.
         */
        size_t variety() const {
            return m_freq.size();
        }

    private:
        T m_min;
        T m_max;

        // ci: coverage fraction of pixel i
        // wi: weight of pixel i
        // xi: value of pixel i
        double m_sum_ciwi;
        double m_sum_ci;
        double m_sum_xici;
        double m_sum_xiciwi;

        std::unordered_map<T, float> m_freq;

        bool m_store_values;

        void process_value(const T& val, float coverage, double weight) {
            double ciwi = static_cast<double>(coverage)*weight;

            m_sum_ci += static_cast<double>(coverage);
            m_sum_ciwi += ciwi;
            m_sum_xici += val*static_cast<double>(coverage);
            m_sum_xiciwi += val*ciwi;

            if (val < m_min) {
                m_min = val;
            }

            if (val > m_max) {
                m_max = val;
            }

            // TODO should weights factor in here?
            if (m_store_values)
                m_freq[val] += coverage;
        }
    };

}

#endif
