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

#include "grid.h"
#include "matrix.h"
#include "raster.h"

namespace exactextract {

    class RasterCellIntersection {

    public:
        RasterCellIntersection(const Grid<bounded_extent> &raster_grid, GEOSContextHandle_t context, const GEOSGeometry *g);

        size_t rows() const { return m_overlap_areas->rows(); }

        size_t cols() const { return m_overlap_areas->cols(); }

        const Matrix<float> &overlap_areas() const { return *m_overlap_areas; }

        Grid<infinite_extent> m_geometry_grid;
    private:
        void process(GEOSContextHandle_t context, const GEOSGeometry *g);

        void process_ring(GEOSContextHandle_t context, const GEOSGeometry *ls, bool exterior_ring);

        void add_ring_areas(size_t i0, size_t j0, const Matrix<float> &areas, bool exterior_ring);

        std::unique_ptr<Matrix<float>> m_overlap_areas;

    };

    Raster<float> raster_cell_intersection(const Grid<bounded_extent> & raster_grid, GEOSContextHandle_t context, const GEOSGeometry* g);

}

#endif
