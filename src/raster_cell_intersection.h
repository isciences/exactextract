// Copyright (c) 2018-2023 ISciences, LLC.
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

#include "geos_utils.h"
#include "grid.h"
#include "matrix.h"
#include "raster.h"

namespace exactextract {

class Cell;

/**
 * @brief The RasterCellIntersection class computes and stores information about intersection of a Grid and a Geometry.
 *        For a polygonal geometry, the fraction of each grid cell covered by polygon is stored.
 *        For a linear geometry, the length of the line in each grid cell is stored.
 *
 *        Intersection information returned as `Raster` with an equivalent resolution to the input `Grid`. The spatial
 *        extent may be reduced from the input `Grid.`
 */
class RasterCellIntersection
{

  public:
    /**
     * @brief Compute an intersection between a grid and a Geometry
     */
    RasterCellIntersection(const Grid<bounded_extent>& raster_grid, GEOSContextHandle_t context, const GEOSGeometry* g);

    /**
     * @brief Compute an intersection between a grid and a Box
     */
    RasterCellIntersection(const Grid<bounded_extent>& raster_grid, const Box& box);

    /**
     * @brief Return the number of rows in the result grid
     */
    size_t rows() const { return m_results->rows(); }

    /**
     * @brief Return the number of columns in the result grid
     */
    size_t cols() const { return m_results->cols(); }

    /**
     * @brief Return the intersection result matrix
     */
    const Matrix<float>& results() const { return *m_results; }

    Grid<infinite_extent> m_geometry_grid;

    /**
     * @brief Partition a polygonal geometry by a grid
     */
    static geom_ptr_r subdivide_polygon(const Grid<bounded_extent>& p_grid, GEOSContextHandle_t context, const GEOSGeometry* g);

  private:
    void process(GEOSContextHandle_t context, const GEOSGeometry* g);

    void process_line(GEOSContextHandle_t context, const GEOSGeometry* ls, bool exterior_ring);

    void process_rectangular_ring(const Box& box, bool exterior_ring);

    void add_ring_results(size_t i0, size_t j0, const Matrix<float>& areas, bool exterior_ring);

    void set_areal(bool areal);

    static Matrix<float> collect_areas(const Matrix<std::unique_ptr<Cell>>& cells,
                                       const Grid<bounded_extent>& finite_ring_grid,
                                       GEOSContextHandle_t context,
                                       const GEOSGeometry* ls);

    std::unique_ptr<Matrix<float>> m_results;
    bool m_first_geom;
    bool m_areal;
};

Raster<float>
raster_cell_intersection(const Grid<bounded_extent>& raster_grid, GEOSContextHandle_t context, const GEOSGeometry* g);
Raster<float>
raster_cell_intersection(const Grid<bounded_extent>& raster_grid, const Box& box);

/**
 * @brief Determines the bounding box of the raster-vector intersection. Considers the bounding boxes
 *        of individual polygon components separately to avoid unnecessary computation for sparse
 *        multi-polygons.
 *
 * @param raster_extent Box representing the extent of the vector
 * @param component_boxes vector of Box objects representing each component of a multipart geometry
 *
 * @return the portion of `raster_extent` that intersects one or more `component_boxes`
 */
Box
processing_region(const Box& raster_extent, const std::vector<Box>& component_boxes);

}

#endif
