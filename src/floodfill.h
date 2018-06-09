#ifndef FLOODFILL_H
#define FLOODFILL_H

#include <queue>

#include <geos_c.h>

#include "extent.h"
#include "geos_utils.h"
#include "matrix.h"

namespace exactextract {

    template<typename T>
    struct fill_values {
        static T EXTERIOR;
    };

    template<>
    struct fill_values<float> {
        static constexpr float EXTERIOR{-1};
        static constexpr float INTERIOR{1};
    };

    class FloodFill {

    public:
        FloodFill(const GEOSGeometry *g, const Extent &extent);

        template<typename T>
        void flood(Matrix<T> &arr) const;

    private:
        Extent m_extent;
        geom_ptr m_g;
        prep_geom_ptr m_pg;

        bool cell_is_inside(size_t i, size_t j) const;

    };

    template<typename T>
    void invert_flood(Matrix<T> &arr) {
        for (size_t i = 0; i < arr.rows(); i++) {
            for (size_t j = 0; j < arr.cols(); j++) {
                if (arr(i, j) == fill_values<T>::EXTERIOR) {
                    arr(i, j) = 0;
                }
            }
        }
    }

    template<typename T>
    void flood_from_pixel(Matrix<T> &arr, size_t i, size_t j, T fill_value) {
        std::queue<std::pair<size_t, size_t> > locations;

        locations.emplace(i, j);

        while (!locations.empty()) {
            i = locations.front().first;
            j = locations.front().second;
            locations.pop();

            if (arr(i, j) == fill_value) {
                continue;
            }

            // Left
            if (j > 0 && arr(i, j - 1) == 0) {
                locations.emplace(i, j - 1);
            }

            auto j0 = j;

            // Fill along this row until we hit something
            for (; j < arr.cols() && arr(i, j) == 0; j++) {
                arr(i, j) = fill_value;
            }

            auto j1 = j;

            // Initiate scanlines above our current row
            if (i > 0) {
                for (j = j0; j < j1; j++) {
                    // Up
                    if (arr(i - 1, j) == 0) {
                        locations.emplace(i - 1, j);
                    }
                }
            }

            // Initiate scanlines below our current row
            if (i < arr.rows() - 1) {
                for (j = j0; j < j1; j++) {
                    // Down
                    if (arr(i + 1, j) == 0) {
                        locations.emplace(i + 1, j);
                    }
                }
            }

        }
    }


    template<typename T>
    void FloodFill::flood(Matrix<T> &arr) const {

        for (size_t i = 0; i < arr.rows(); i++) {
            for (size_t j = 0; j < arr.cols(); j++) {
                if (arr(i, j) == 0) {
                    if (cell_is_inside(i, j)) {
                        flood_from_pixel(arr, i, j, fill_values<T>::INTERIOR);
                    } else {
                        flood_from_pixel(arr, i, j, fill_values<T>::EXTERIOR);
                    }
                }
            }
        }

        invert_flood(arr);
    }

}

#endif