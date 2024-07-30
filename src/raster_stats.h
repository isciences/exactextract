// Copyright (c) 2018-2024 ISciences, LLC.
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

#include <algorithm>
#include <limits>
#include <optional>
#include <unordered_map>

#include "raster_area.h"
#include "variance.h"
#include "weighted_quantiles.h"

namespace exactextract {

enum class CoverageWeightType
{
    FRACTION,
    NONE,
    AREA_CARTESIAN,
    AREA_SPHERICAL_M2,
    AREA_SPHERICAL_KM2,
};

struct RasterStatsOptions
{
    static constexpr float min_coverage_fraction_default = std::numeric_limits<float>::min(); // ~1e-38

    float min_coverage_fraction = min_coverage_fraction_default;
    bool calc_variance = false;
    bool store_histogram = false;
    bool store_values = false;
    bool store_weights = false;
    bool store_coverage_fraction = false;
    bool store_xy = false;
    CoverageWeightType weight_type = CoverageWeightType::FRACTION;
    double default_weight = std::numeric_limits<double>::quiet_NaN();
};

template<typename T>
struct RasterStatsOptionsWithDefault : public RasterStatsOptions
{
    RasterStatsOptionsWithDefault(const RasterStatsOptions& opts)
      : RasterStatsOptions(opts)
    {
    }

    std::optional<T> default_value = std::nullopt;
};

template<typename T>
class RasterStats
{
  public:
    using ValueType = T;

    /**
     * Compute raster statistics from a Raster representing intersection percentages,
     * a Raster representing data values, and (optionally) a Raster representing weights.
     * and a set of raster values.
     */
    RasterStats(RasterStatsOptionsWithDefault<T> options = RasterStatsOptions{})
      : m_min{ std::numeric_limits<T>::max() }
      , m_max{ std::numeric_limits<T>::lowest() }
      , m_sum_ciwi{ 0 }
      , m_sum_ci{ 0 }
      , m_sum_xici{ 0 }
      , m_sum_xiciwi{ 0 }
      , m_options{ options }
    {
    }

    static bool get_or_default(const AbstractRaster<T>& r, std::size_t i, std::size_t j, T& val, const std::optional<T>& default_value)
    {
        if (r.get(i, j, val)) {
            return true;
        }

        if (default_value.has_value()) {
            val = default_value.value();
            return true;
        }

        return false;
    }

    void process(const Raster<float>& intersection_percentages, const AbstractRaster<T>& rast)
    {
        std::unique_ptr<AbstractRaster<T>> rvp;

        if (rast.grid() != intersection_percentages.grid()) {
            rvp = std::make_unique<RasterView<T>>(rast, intersection_percentages.grid());
        }

        const AbstractRaster<T>& rv = rvp ? *rvp : rast;
        std::unique_ptr<AbstractRaster<float>> areas = area_raster(intersection_percentages.grid(), m_options.weight_type);

        for (size_t i = 0; i < rv.rows(); i++) {
            for (size_t j = 0; j < rv.cols(); j++) {
                float pct_cov = intersection_percentages(i, j);
                T val;
                if (pct_cov >= m_options.min_coverage_fraction && get_or_default(rv, i, j, val, m_options.default_value)) {

                    if (areas) {
                        pct_cov *= (*areas)(i, j);
                    }

                    process_location(intersection_percentages.grid(), i, j);
                    process_value(val, pct_cov, 1.0);
                }
            }
        }
    }

    template<typename WeightType>
    void process(const Raster<float>& intersection_percentages, const AbstractRaster<ValueType>& rast, const AbstractRaster<WeightType>& weights)
    {
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
        std::unique_ptr<AbstractRaster<ValueType>> rvp;
        std::unique_ptr<AbstractRaster<WeightType>> wvp;
        std::unique_ptr<AbstractRaster<float>> areas = area_raster(common, m_options.weight_type);

        if (rast.grid() != common) {
            rvp = std::make_unique<RasterView<ValueType>>(rast, common);
        }

        if (weights.grid() != common) {
            wvp = std::make_unique<RasterView<WeightType>>(weights, common);
        }

        const AbstractRaster<ValueType>& rv = rvp ? *rvp : rast;
        const AbstractRaster<WeightType>& wv = wvp ? *wvp : weights;

        for (size_t i = 0; i < rv.rows(); i++) {
            for (size_t j = 0; j < rv.cols(); j++) {
                float pct_cov = intersection_percentages(i, j);
                WeightType weight;
                ValueType val;

                if (pct_cov >= m_options.min_coverage_fraction && get_or_default(rv, i, j, val, m_options.default_value)) {
                    process_location(common, i, j);

                    if (areas) {
                        pct_cov *= (*areas)(i, j);
                    }
                    if (wv.get(i, j, weight)) {
                        process_value(val, pct_cov, static_cast<double>(weight));
                    } else {
                        // Weight is NODATA
                        process_value(val, pct_cov, m_options.default_weight);
                    }
                }
            }
        }
    }

    void process_location(const Grid<bounded_extent>& grid, std::size_t row, std::size_t col)
    {
        if (m_options.store_xy) {
            m_cell_x.push_back(grid.x_for_col(col));
            m_cell_y.push_back(grid.y_for_row(row));
        }
    }

    void process_value(const T& val, float coverage, double weight)
    {
        if (m_options.weight_type == CoverageWeightType::NONE) {
            coverage = 1.0f;
        }

        if (m_options.store_coverage_fraction) {
            m_cell_cov.push_back(coverage);
        }

        m_sum_ci += static_cast<double>(coverage);
        m_sum_xici += static_cast<double>(val) * static_cast<double>(coverage);

        double ciwi = static_cast<double>(coverage) * weight;
        m_sum_ciwi += ciwi;
        m_sum_xiciwi += static_cast<double>(val) * ciwi;

        if (m_options.calc_variance) {
            m_variance.process(static_cast<double>(val), static_cast<double>(coverage));
            m_weighted_variance.process(static_cast<double>(val), ciwi);
        }

        if (val < m_min) {
            m_min = val;
            if (m_options.store_xy) {
                m_min_xy = { m_cell_x.back(), m_cell_y.back() };
            }
        }

        if (val > m_max) {
            m_max = val;
            if (m_options.store_xy) {
                m_max_xy = { m_cell_x.back(), m_cell_y.back() };
            }
        }

        if (m_options.store_histogram) {
            auto& entry = m_freq[val];
            entry.m_sum_ci += static_cast<double>(coverage);
            entry.m_sum_ciwi += ciwi;
            m_quantiles.reset();
        }

        if (m_options.store_values) {
            m_cell_values.push_back(val);
        }

        if (m_options.store_weights) {
            m_cell_weights.push_back(weight);
        }
    }

    static std::unique_ptr<AbstractRaster<float>> area_raster(const Grid<bounded_extent>& grid, CoverageWeightType method)
    {
        switch (method) {
            case CoverageWeightType::AREA_SPHERICAL_M2:
                return std::make_unique<SphericalAreaRaster<float>>(grid, AreaUnit::M2);
            case CoverageWeightType::AREA_SPHERICAL_KM2:
                return std::make_unique<SphericalAreaRaster<float>>(grid, AreaUnit::KM2);
            case CoverageWeightType::AREA_CARTESIAN:
                return std::make_unique<ConstantRaster<float>>(grid, grid.dx() * grid.dy());
            case CoverageWeightType::NONE:
            case CoverageWeightType::FRACTION:
                return nullptr;
        }
        return nullptr;
    }

    /**
     * The mean value of cells covered by this polygon, weighted
     * by the percent of the cell that is covered.
     */
    double
    mean() const
    {
        if (count() > 0) {
            return sum() / count();
        } else {
            return std::numeric_limits<double>::quiet_NaN();
        }
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
    double
    weighted_mean() const
    {
        if (weighted_count()) {
            return weighted_sum() / weighted_count();
        } else {
            return std::numeric_limits<double>::quiet_NaN();
        }
    }

    /** The fraction of weighted cells to unweighted cells.
     *  Meaningful only when the values of the weighting
     *  raster are between 0 and 1.
     */
    double
    weighted_fraction() const
    {
        return weighted_sum() / sum();
    }

    /**
     * The raster value occupying the greatest number of cells
     * or partial cells within the polygon. When multiple values
     * cover the same number of cells, the greatest value will
     * be returned. Weights are not taken into account.
     */
    std::optional<T>
    mode() const
    {
        if (variety() == 0) {
            return std::nullopt;
        }

        return std::max_element(m_freq.cbegin(),
                                m_freq.cend(),
                                [](const auto& a, const auto& b) {
                                    return a.second.m_sum_ci < b.second.m_sum_ci || (a.second.m_sum_ci == b.second.m_sum_ci && a.first < b.first);
                                })
          ->first;
    }

    /**
     * The minimum value in any raster cell wholly or partially covered
     * by the polygon. Weights are not taken into account.
     */
    std::optional<T>
    min() const
    {
        if (m_sum_ci == 0) {
            return std::nullopt;
        }
        return m_min;
    }

    /// XY values corresponding to the center of the cell whose value
    /// is returned by min()
    std::optional<std::pair<double, double>>
    min_xy() const
    {
        if (m_sum_ci == 0) {
            return std::nullopt;
        }
        return m_min_xy;
    }

    /**
     * The maximum value in any raster cell wholly or partially covered
     * by the polygon. Weights are not taken into account.
     */
    std::optional<T>
    max() const
    {
        if (m_sum_ci == 0) {
            return std::nullopt;
        }
        return m_max;
    }

    /// XY values corresponding to the center of the cell whose value
    /// is returned by max()
    std::optional<std::pair<double, double>>
    max_xy() const
    {
        if (m_sum_ci == 0) {
            return std::nullopt;
        }
        return m_max_xy;
    }

    /**
     * The given quantile (0-1) of raster cell values. Coverage fractions
     * are taken into account but weights are not.
     */
    std::optional<T>
    quantile(double q) const
    {
        if (m_sum_ci == 0) {
            return std::nullopt;
        }

        // The weighted quantile computation is not processed incrementally.
        // Create it on demand and retain it in case we want multiple quantiles.
        if (!m_quantiles) {
            m_quantiles = std::make_unique<WeightedQuantiles>();

            for (const auto& entry : m_freq) {
                m_quantiles->process(static_cast<double>(entry.first), entry.second.m_sum_ci);
            }
        }

        return m_quantiles->quantile(q);
    }

    /**
     * The sum of raster cells covered by the polygon, with each raster
     * value weighted by its coverage fraction.
     */
    double
    sum() const
    {
        return m_sum_xici;
    }

    /**
     * The sum of raster cells covered by the polygon, with each raster
     * value weighted by its coverage fraction and weighting raster value.
     *
     * If any weights are undefined, will return NAN. If this is undesirable,
     * caller should replace undefined weights with a suitable default
     * before computing statistics.
     */
    double
    weighted_sum() const
    {
        return m_sum_xiciwi;
    }

    /**
     * The number of raster cells with any defined value
     * covered by the polygon. Weights are not taken
     * into account.
     */
    double
    count() const
    {
        return m_sum_ci;
    }

    /**
     * The number of raster cells with a specific value
     * covered by the polygon. Weights are not taken
     * into account.
     */
    std::optional<double>
    count(const T& value) const
    {
        const auto& entry = m_freq.find(value);

        if (entry == m_freq.end()) {
            return std::nullopt;
        }

        return entry->second.m_sum_ci;
    }

    /**
     * The fraction of defined raster cells covered by the polygon with
     * a value that equals the specified value.
     * Weights are not taken into account.
     */
    std::optional<double>
    frac(const T& value) const
    {
        auto count_for_value = count(value);

        if (!count_for_value.has_value()) {
            return count_for_value;
        }

        return count_for_value.value() / count();
    }

    /**
     * The weighted fraction of defined raster cells covered by the polygon with
     * a value that equals the specified value.
     */
    std::optional<double>
    weighted_frac(const T& value) const
    {
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
    double
    variance() const
    {
        return m_variance.variance();
    }

    /**
     * The population variance of raster cells touched
     * by the polygon, taking into account cell coverage
     * fractions and values of a weighting raster.
     */
    double
    weighted_variance() const
    {
        return m_weighted_variance.variance();
    }

    /**
     * The population standard deviation of raster cells
     * touched by the polygon. Cell coverage fractions
     * are taken into account; values of a weighting
     * raster are not.
     */
    double
    stdev() const
    {
        return m_variance.stdev();
    }

    /**
     * The population standard deviation of raster cells
     * touched by the polygon, taking into account cell
     * coverage fractions and values of a weighting raster.
     */
    double
    weighted_stdev() const
    {
        return m_weighted_variance.stdev();
    }

    /**
     * The population coefficient of variation of raster
     * cells touched by the polygon. Cell coverage fractions
     * are taken into account; values of a weighting
     * raster are not.
     */
    double
    coefficient_of_variation() const
    {
        return m_variance.coefficent_of_variation();
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
    double
    weighted_count() const
    {
        return m_sum_ciwi;
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
    std::optional<double>
    weighted_count(const T& value) const
    {
        const auto& entry = m_freq.find(value);

        if (entry == m_freq.end()) {
            return std::nullopt;
        }

        return entry->second.m_sum_ciwi;
    }

    /**
     * The raster value occupying the least number of cells
     * or partial cells within the polygon. When multiple values
     * cover the same number of cells, the lowest value will
     * be returned.
     *
     * Cell weights are not taken into account.
     */
    std::optional<T>
    minority() const
    {
        if (variety() == 0) {
            return std::nullopt;
        }

        return std::min_element(m_freq.cbegin(),
                                m_freq.cend(),
                                [](const auto& a, const auto& b) {
                                    return a.second.m_sum_ci < b.second.m_sum_ci || (a.second.m_sum_ci == b.second.m_sum_ci && a.first < b.first);
                                })
          ->first;
    }

    /**
     * The number of distinct defined raster values in cells wholly
     * or partially covered by the polygon.
     */
    std::uint64_t
    variety() const
    {
        return m_freq.size();
    }

    const std::vector<ValueType>&
    values() const
    {
        return m_cell_values;
    }

    const std::vector<float>&
    coverage_fractions() const
    {
        return m_cell_cov;
    }

    const std::vector<double>&
    weights() const
    {
        return m_cell_weights;
    }

    const std::vector<double>&
    center_x() const
    {
        return m_cell_x;
    }

    const std::vector<double>&
    center_y() const
    {
        return m_cell_y;
    }

  private:
    T m_min;
    T m_max;
    std::pair<double, double> m_min_xy;
    std::pair<double, double> m_max_xy;

    // ci: coverage fraction of pixel i
    // wi: weight of pixel i
    // xi: value of pixel i
    double m_sum_ciwi;
    double m_sum_ci;
    double m_sum_xici;
    double m_sum_xiciwi;
    WestVariance m_variance;
    WestVariance m_weighted_variance;

    mutable std::unique_ptr<WeightedQuantiles> m_quantiles;

    struct ValueFreqEntry
    {
        double m_sum_ci = 0;
        double m_sum_ciwi = 0;
    };
    std::unordered_map<T, ValueFreqEntry> m_freq;

    std::vector<float> m_cell_cov;
    std::vector<T> m_cell_values;
    std::vector<double> m_cell_weights;
    std::vector<double> m_cell_x;
    std::vector<double> m_cell_y;

    const RasterStatsOptionsWithDefault<T> m_options;

    struct Iterator
    {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = const T;
        using pointer = value_type*;
        using reference = value_type&;

        using underlying_iterator = decltype(m_freq.cbegin());

        Iterator(underlying_iterator it)
          : m_iterator(it)
        {
        }

        reference operator*() const
        {
            return m_iterator->first;
        }

        pointer operator->() const
        {
            return &(m_iterator->first);
        }

        // prefix
        Iterator& operator++()
        {
            m_iterator++;
            return *this;
        }

        // postfix
        Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const Iterator& a, const Iterator& b)
        {
            return a.m_iterator == b.m_iterator;
        }
        friend bool operator!=(const Iterator& a, const Iterator& b)
        {
            return !(a == b);
        }

      private:
        underlying_iterator m_iterator;
    };

  public:
    Iterator
    begin() const
    {
        return Iterator(m_freq.cbegin());
    }

    Iterator
    end() const
    {
        return Iterator(m_freq.cend());
    }
};

template<typename T>
std::ostream&
operator<<(std::ostream& os, const RasterStats<T>& stats)
{
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
