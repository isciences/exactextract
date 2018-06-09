#ifndef RASTER_STATS_H
#define RASTER_STATS_H

#include <unordered_map>
#include "raster_cell_intersection.h"

namespace exactextract {

    class RasterStats {

    public:
        RasterStats(const RasterCellIntersection &rci, const Matrix<float> rast) :
                m_max{std::numeric_limits<float>::lowest()},
                m_min{std::numeric_limits<float>::max()},
                m_rows{rci.rows()},
                m_cols{rci.cols()},
                m_weights{0},
                m_weighted_vals{0} {
            for (size_t i = 0; i < m_rows; i++) {
                for (size_t j = 0; j < m_cols; j++) {
                    float val = rast(i, j);
                    float weight = rci.get_local(i, j);

                    if (!std::isnan(val)) {
                        m_weights += weight;
                        m_weighted_vals += weight * val;


                        m_freq[val] += m_weights;
                    }


                }
            }


        }

        float mean() {
            return sum() / count();
        }

        float mode() {
            return std::max_element(std::cbegin(m_freq),
                                    std::cend(m_freq),
                                    [](const auto &a, const auto &b) {
                                        return a->second < b->second;
                                    })->first;
        }

        float min() {
            return m_min;
        }

        float max() {
            return m_max;
        }

        float sum() {
            return (float) m_weighted_vals;
        }

        float count() {
            return (float) m_weights;
        }

        float minority() {
            return std::min_element(std::cbegin(m_freq),
                                    std::cend(m_freq),
                                    [](const auto &a, const auto &b) {
                                        return a->second < b->second;
                                    })->first;
        }

        size_t variety() {
            return m_freq.size();
        }

    private:
        float m_min;
        float m_max;

        double m_weights;
        double m_weighted_vals;

        std::unordered_map<float, float> m_freq;

        size_t m_rows;
        size_t m_cols;

    };

}

#endif