#ifndef RASTER_STATS_H
#define RASTER_STATS_H

#include <algorithm>
#include <limits>
#include <unordered_map>

#include "raster_cell_intersection.h"

namespace exactextract {

    template<typename T>
    class RasterStats {

    public:
        RasterStats(const RasterCellIntersection & rci, const Matrix<T> & rast, const T* nodata = nullptr) :
                m_max{std::numeric_limits<T>::lowest()},
                m_min{std::numeric_limits<T>::max()},
                m_rows{rci.rows()},
                m_cols{rci.cols()},
                m_weights{0},
                m_weighted_vals{0} {

            for (size_t i = rci.min_row(); i < rci.max_row(); i++) {
                for (size_t j = rci.min_col(); j < rci.max_col(); j++) {
                    T val = rast(i, j);
                    float weight = rci.get(i, j);

                    if (weight > 0 && !std::isnan(val) && (nodata == nullptr || val != *nodata)) {
                        m_weights += weight;
                        m_weighted_vals += weight * val;

                        if (val < m_min) {
                            m_min = val;
                        }

                        if (val > m_max) {
                            m_max = val;
                        }

                        m_freq[val] += weight;
                    }


                }
            }


        }

        /**
         * The mean value of cells covered by this polygon, weighted
         * by the percent of the cell that is covered.
         */
        float mean() {
            return sum() / count();
        }

        /**
         * The raster value occupying the greatest number of cells
         * or partial cells within the polygon. When multiple values
         * cover the same number of cells, the greatest value will
         * be returned.
         */
        T mode() {
            return std::max_element(std::cbegin(m_freq),
                                    std::cend(m_freq),
                                    [](const auto &a, const auto &b) {
                                        return a.second < b.second || (a.second == b.second && a.first < b.first);
                                    })->first;
        }

        /**
         * The minimum value in any raster cell wholly or partially covered
         * by the polygon.
         */
        T min() {
            return m_min;
        }

        /**
         * The maximum value in any raster cell wholly or partially covered
         * by the polygon.
         */
        T max() {
            return m_max;
        }

        /**
         * The weighted sum of raster cells covered by the polygon.
         */
        float sum() {
            return (float) m_weighted_vals;
        }

        /**
         * The number of raster cells with a defined value
         * covered by the polygon.
         */
        float count() {
            return (float) m_weights;
        }

        /**
         * The raster value occupying the least number of cells
         * or partial cells within the polygon. When multiple values
         * cover the same number of cells, the lowest value will
         * be returned.
         */
        T minority() {
            return std::min_element(std::cbegin(m_freq),
                                    std::cend(m_freq),
                                    [](const auto &a, const auto &b) {
                                        return a.second < b.second || (a.second == b.second && a.first < b.first);
                                    })->first;
        }

        /**
         * The number of distinct defined raster values in cells wholly
         * or partially covered by the polygon.
         */
        size_t variety() {
            return m_freq.size();
        }

    private:
        T m_min;
        T m_max;

        double m_weights;
        double m_weighted_vals;

        std::unordered_map<T, float> m_freq;

        size_t m_rows;
        size_t m_cols;
    };

}

#endif