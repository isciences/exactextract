// Copyright (c) 2018-2022 ISciences, LLC.
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
#include <iostream>
#include <limits>
#include <unordered_map>

#include "raster_cell_intersection.h"
#include "weighted_quantiles.h"
#include "variance.h"

#include "../vend/optional.hpp"

namespace exactextract {

    template<typename T>
    class RasterStats {

    public:
        /**
         * Compute raster statistics from a Raster representing intersection percentages,
         * a Raster representing data values, and (optionally) a Raster representing weights.
         * and a set of raster values.
         */
        explicit RasterStats(bool store_values = false) :
                m_min{std::numeric_limits<T>::max()},
                m_max{std::numeric_limits<T>::lowest()},
                m_sum_ciwi{0},
                m_sum_ci{0},
                m_sum_xici{0},
                m_sum_xiciwi{0},
                m_store_values{store_values} {}

        void process(const Raster<float> & intersection_percentages, const AbstractRaster<T> & rast) {
            std::unique_ptr<AbstractRaster<T>> rvp;

            if (rast.grid() != intersection_percentages.grid()) {
                rvp = std::make_unique<RasterView<T>>(rast, intersection_percentages.grid());
            }

            const AbstractRaster<T>& rv = rvp ? *rvp : rast;

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
            // Process the entire intersection_percentages grid, even though it may
            // be outside the extent of the weighting raster. Although we've been
            // provided a weighting raster, we still need to calculate correct values
            // for unweighted stats.
            auto& common = intersection_percentages.grid();

            if (common.empty())
                return;

            // If the value or weights grids do not correspond to the intersection_percentages grid,
            // construct a RasterView to perform the transformation. Even a no-op RasterView can be
            // expensive, so we avoid doing this unless necessary.
            std::unique_ptr<AbstractRaster<T>> rvp;
            std::unique_ptr<AbstractRaster<T>> wvp;

            if (rast.grid() != common) {
                rvp = std::make_unique<RasterView<T>>(rast, common);
            }

            if (weights.grid() != common) {
                wvp = std::make_unique<RasterView<T>>(weights, common);
            }

            const AbstractRaster<T>& rv = rvp ? *rvp : rast;
            const AbstractRaster<T>& wv = wvp ? *wvp : weights;

            for (size_t i = 0; i < rv.rows(); i++) {
                for (size_t j = 0; j < rv.cols(); j++) {
                    float pct_cov = intersection_percentages(i, j);
                    T weight;
                    T val;

                    if (pct_cov > 0 && rv.get(i, j, val)) {
                        if (wv.get(i, j, weight)) {
                            process_value(val, pct_cov, weight);
                        } else {
                            // Weight is NODATA, convert to NAN
                            process_value(val, pct_cov, std::numeric_limits<double>::quiet_NaN());
                        }
                    }
                }
            }
        }

        void process_value(const T& val, float coverage, double weight) {
            m_sum_ci += static_cast<double>(coverage);
            m_sum_xici += val*static_cast<double>(coverage);

            m_variance.process(val, coverage);

            double ciwi = static_cast<double>(coverage)*weight;
            m_sum_ciwi += ciwi;
            m_sum_xiciwi += val * ciwi;

            if (val < m_min) {
                m_min = val;
            }

            if (val > m_max) {
                m_max = val;
            }

            if (m_store_values) {
                auto& entry = m_freq[val];
                entry.m_sum_ci += static_cast<double>(coverage);
                entry.m_sum_ciwi += ciwi;
                m_quantiles.reset();
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
         *
         * If any weights are undefined, will return NAN. If this is undesirable,
         * caller should replace undefined weights with a suitable default
         * before computing statistics.
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
        nonstd::optional<T> mode() const {
            if (variety() == 0) {
                return nonstd::nullopt;
            }

            return std::max_element(m_freq.cbegin(),
                                    m_freq.cend(),
                                    [](const auto &a, const auto &b) {
                                        return a.second.m_sum_ci < b.second.m_sum_ci || (a.second.m_sum_ci == b.second.m_sum_ci && a.first < b.first);
                                    })->first;
        }

        /**
         * The minimum value in any raster cell wholly or partially covered
         * by the polygon. Weights are not taken into account.
         */
        nonstd::optional<T> min() const {
            if (m_sum_ci == 0) {
                return nonstd::nullopt;
            }
            return m_min;
        }

        /**
         * The maximum value in any raster cell wholly or partially covered
         * by the polygon. Weights are not taken into account.
         */
        nonstd::optional<T> max() const {
            if (m_sum_ci == 0) {
                return nonstd::nullopt;
            }
            return m_max;
        }

        /**
         * The given quantile (0-1) of raster cell values. Coverage fractions
         * are taken into account but weights are not.
         */
        nonstd::optional<T> quantile(double q) const {
            if (m_sum_ci == 0) {
                return nonstd::nullopt;
            }

            // The weighted quantile computation is not processed incrementally.
            // Create it on demand and retain it in case we want multiple quantiles.
            if (!m_quantiles) {
                m_quantiles = std::make_unique<WeightedQuantiles>();

                for (const auto& entry : m_freq) {
                    m_quantiles->process(entry.first, entry.second.m_sum_ci);
                }
            }

            return m_quantiles->quantile(q);
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
         *
         * If any weights are undefined, will return NAN. If this is undesirable,
         * caller should replace undefined weights with a suitable default
         * before computing statistics.
         */
        float weighted_sum() const {
            return (float) m_sum_xiciwi;
        }

        /**
         * The number of raster cells with any defined value
         * covered by the polygon. Weights are not taken
         * into account.
         */
        float count() const {
            return (float) m_sum_ci;
        }

        /**
         * The number of raster cells with a specific value
         * covered by the polygon. Weights are not taken
         * into account.
         */
        nonstd::optional<float> count(const T& value) const {
            const auto& entry = m_freq.find(value);

            if (entry == m_freq.end()) {
                return nonstd::nullopt;
            }

            return static_cast<float>(entry->second.m_sum_ci);
        }

        /**
         * The fraction of defined raster cells covered by the polygon with
         * a value that equals the specified value.
         * Weights are not taken into account.
         */
        nonstd::optional<float> frac(const T& value) const {
            auto count_for_value = count(value);

            if (!count_for_value.has_value()) {
                return count_for_value;
            }

            return count_for_value.value() / count();
        }

        /**
         * The weighted fraction of defined raster cells covered by the polygon with
         * a value that equals the specified value.
         * Weights are not taken into account.
         */
        nonstd::optional<float> weighted_frac(const T& value) const {
            auto count_for_value = weighted_count(value);

            if (!count_for_value.has_value()) {
                return count_for_value;
            }

            return count_for_value.value() / weighted_count();
        }

        /**
         * The population variance of raster cells touched
         * by the polygon. Cell coverage fractions are taken
         * into account; values of a weighting raster are not.
         */
        float variance() const {
            return static_cast<float>(m_variance.variance());
        }

        /**
         * The population standard deviation of raster cells
         * touched by the polygon. Cell coverage fractions
         * are taken into account; values of a weighting
         * raster are not.
         */
        float stdev() const {
            return static_cast<float>(m_variance.stdev());
        }

        /**
         * The population coefficient of variation of raster
         * cells touched by the polygon. Cell coverage fractions
         * are taken into account; values of a weighting
         * raster are not.
         */
        float coefficient_of_variation() const {
            return static_cast<float>(m_variance.coefficent_of_variation());
        }

        /**
         * The sum of weights for each cell covered by the
         * polygon, with each weight multiplied by the coverage
         * coverage fraction of each cell.
         *
         * If any weights are undefined, will return NAN. If this is undesirable,
         * caller should replace undefined weights with a suitable default
         * before computing statistics.
         */
        float weighted_count() const {
            return (float) m_sum_ciwi;
        }

        /**
         * The sum of weights for each cell of a specific value covered by the
         * polygon, with each weight multiplied by the coverage coverage fraction
         * of each cell.
         *
         * If any weights are undefined, will return NAN. If this is undesirable,
         * caller should replace undefined weights with a suitable default
         * before computing statistics.
         */
        nonstd::optional<float> weighted_count(const T& value) const {
            const auto& entry = m_freq.find(value);

            if (entry == m_freq.end()) {
                return nonstd::nullopt;
            }

            return static_cast<float>(entry->second.m_sum_ciwi);
        }

        /**
         * The raster value occupying the least number of cells
         * or partial cells within the polygon. When multiple values
         * cover the same number of cells, the lowest value will
         * be returned.
         *
         * Cell weights are not taken into account.
         */
        nonstd::optional<T> minority() const {
            if (variety() == 0) {
                return nonstd::nullopt;
            }

            return std::min_element(m_freq.cbegin(),
                                    m_freq.cend(),
                                    [](const auto &a, const auto &b) {
                                        return a.second.m_sum_ci < b.second.m_sum_ci || (a.second.m_sum_ci == b.second.m_sum_ci && a.first < b.first);
                                    })->first;
        }

        /**
         * The number of distinct defined raster values in cells wholly
         * or partially covered by the polygon.
         */
        size_t variety() const {
            return m_freq.size();
        }

        bool stores_values() const {
            return m_store_values;
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
        WestVariance m_variance;

        mutable std::unique_ptr<WeightedQuantiles> m_quantiles;

        struct ValueFreqEntry {
            double m_sum_ci = 0;
            double m_sum_ciwi = 0;
        };
        std::unordered_map<T, ValueFreqEntry> m_freq;

        bool m_store_values;

        struct Iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = const T;
            using pointer = value_type*;
            using reference = value_type&;

            using underlying_iterator = decltype(m_freq.cbegin());

            Iterator(underlying_iterator it) : m_iterator(it) {}

            reference operator*() const {
                return m_iterator->first;
            }

            pointer operator->() const {
                return &(m_iterator->first);
            }

            // prefix
            Iterator& operator++() {
                m_iterator++;
                return *this;
            }

            // postfix
            Iterator operator++(int) {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            friend bool operator==(const Iterator& a, const Iterator& b) {
                return a.m_iterator == b.m_iterator;
            }
            friend bool operator!=(const Iterator& a, const Iterator& b) {
                return !(a == b);
            }

        private:
            underlying_iterator m_iterator;
        };

    public:
        Iterator begin() const {
            return Iterator(m_freq.cbegin());
        }

        Iterator end() const {
            return Iterator(m_freq.cend());
        }

    };

    template<typename T>
    std::ostream& operator<<(std::ostream& os, const RasterStats<T> & stats) {
        os << "{" << std::endl;
        os << "  \"count\" : " << stats.count() << "," << std::endl;

        os << "  \"min\" : ";
        if (stats.min().has_value()) {
            os << stats.min().value();
        } else {
            os << "null";
        }
        os << "," << std::endl;

        os << "  \"max\" : ";
        if (stats.max().has_value()) {
            os << stats.max().value();
        } else {
            os << "null";
        }
        os << "," << std::endl;

        os << "  \"mean\" : " << stats.mean() << "," << std::endl;
        os << "  \"sum\" : " << stats.sum() << "," << std::endl;
        os << "  \"weighted_mean\" : " << stats.weighted_mean() << "," << std::endl;
        os << "  \"weighted_sum\" : " << stats.weighted_sum();
        if (stats.stores_values()) {
            os << "," << std::endl;
            os << "  \"mode\" : ";
            if (stats.mode().has_value()) {
                os << stats.mode().value();
            } else {
                os << "null";
            }
            os << "," << std::endl;

            os << "  \"minority\" : ";
            if (stats.minority().has_value()) {
                os << stats.minority().value();
            } else {
                os << "null";
            }
            os << "," << std::endl;

            os << "  \"variety\" : " << stats.variety() << std::endl;
        } else {
            os << std::endl;
        }
        os << "}" << std::endl;
        return os;
    }


}

#endif
