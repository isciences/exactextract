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

#ifndef EXACTEXTRACT_RASTER_CELL_INTERSECTION_H
#define EXACTEXTRACT_RASTER_CELL_INTERSECTION_H

#include <memory>

#include <geos_c.h>

#include "extent.h"
#include "matrix.h"
#include "raster.h"

namespace exactextract {

    class RasterCellIntersection {

    public:
        RasterCellIntersection(const Extent &raster_extent, const GEOSGeometry *g);

        size_t min_row() const { return m_min_row; }

        size_t min_col() const { return m_min_col; }

        size_t max_row() const { return m_max_row; }

        size_t max_col() const { return m_max_col; }

        size_t rows() const { return max_row() - min_row(); }

        size_t cols() const { return max_col() - min_col(); }

        float get(size_t i, size_t j) const { return m_overlap_areas->operator()(i - m_min_row, j - m_min_col); }

        float get_local(size_t i, size_t j) const { return m_overlap_areas->operator()(i, j); }

        const Matrix<float> &overlap_areas() const { return *m_overlap_areas; }

        Extent m_geometry_extent;
    private:
        size_t m_min_row;
        size_t m_min_col;
        size_t m_max_row;
        size_t m_max_col;

        void process(const GEOSGeometry *g);

        void process_ring(const GEOSGeometry *ls, bool exterior_ring);

        void add_ring_areas(size_t i0, size_t j0, const Matrix<float> &areas, bool exterior_ring);

        std::unique_ptr<Matrix<float>> m_overlap_areas;

    };

    Raster<float> raster_cell_intersection(const Extent & raster_extent, const GEOSGeometry* g);

}

#endif