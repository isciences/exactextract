// Copyright (c) 2018-2020 ISciences, LLC.
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

#ifndef EXACTEXTRACT_FLOODFILL_H
#define EXACTEXTRACT_FLOODFILL_H

#include <queue>

#include <geos_c.h>

#include "grid.h"
#include "geos_utils.h"
#include "matrix.h"

namespace exactextract {

    template<typename T>
    struct fill_values {
        static T EXTERIOR;
    };

    template<>
    struct fill_values<float> {
        static constexpr float EXTERIOR{0.0f};  // Cell is known to be entirely outside the polygon
        static constexpr float INTERIOR{1.0f};  // Cell is known to be entirely within the polygon
        static constexpr float FILLABLE{-1.0f}; // Cell location relative to polygon unknown, but
                                                // can be determined by fill.
        static constexpr float UNKNOWN{-2.0f};  // Cell location relative to polygon unknown
                                                // and cannot be determined from a flood fill
                                                // (must be explicitly tested)

    };

    class FloodFill {

    public:
        FloodFill(GEOSContextHandle_t context, const GEOSGeometry *g, const Grid<bounded_extent> &extent);

        template<typename T>
        void flood(Matrix<T> &arr) const;

        bool cell_is_inside(size_t i, size_t j) const;

    private:
        Grid<bounded_extent> m_extent;
        GEOSContextHandle_t m_geos_context;
        geom_ptr_r m_g;
        prep_geom_ptr_r m_pg;
    };

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
            if (j > 0 && arr(i, j - 1) == fill_values<T>::FILLABLE) {
                locations.emplace(i, j - 1);
            }

            auto j0 = j;

            // Fill along this row until we hit something
            for (; j < arr.cols() && arr(i, j) == fill_values<T>::FILLABLE; j++) {
                arr(i, j) = fill_value;
            }

            auto j1 = j;

            // Initiate scanlines above our current row
            if (i > 0) {
                for (j = j0; j < j1; j++) {
                    // Up
                    if (arr(i - 1, j) == fill_values<T>::FILLABLE) {
                        locations.emplace(i - 1, j);
                    }
                }
            }

            // Initiate scanlines below our current row
            if (i < arr.rows() - 1) {
                for (j = j0; j < j1; j++) {
                    // Down
                    if (arr(i + 1, j) == fill_values<T>::FILLABLE) {
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
                if (arr(i, j) == fill_values<T>::UNKNOWN) {
                    throw std::runtime_error("Cell with unknown position encountered.");
                } else if (arr(i, j) == fill_values<T>::FILLABLE) {
                    // Cell position relative to polygon is unknown but can
                    // be determined from adjacent cells.
                    if (cell_is_inside(i, j)) {
                        flood_from_pixel(arr, i, j, fill_values<T>::INTERIOR);
                    } else {
                        flood_from_pixel(arr, i, j, fill_values<T>::EXTERIOR);
                    }
                }
            }
        }
    }

}

#endif