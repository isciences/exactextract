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

#include <stdexcept>

#include <geos_c.h>

#include "cell.h"
#include "floodfill.h"
#include "geos_utils.h"
#include "raster_cell_intersection.h"
#include "segment_orientation.h"

namespace exactextract {

    Raster<float> raster_cell_intersection(const Grid<bounded_extent> & raster_grid, GEOSContextHandle_t context, const GEOSGeometry* g) {
        RasterCellIntersection rci(raster_grid, context, g);

        return { std::move(const_cast<Matrix<float>&>(rci.overlap_areas())),
                 make_finite(rci.m_geometry_grid) };
    }

    static Cell *get_cell(Matrix<std::unique_ptr<Cell>> &cells, const Grid<infinite_extent> &ex, size_t row, size_t col) {
        //std::cout << " getting cell " << row << ", " << col << std::endl;

        if (cells(row, col) == nullptr) {
            cells(row, col) = std::make_unique<Cell>(grid_cell(ex, row, col));
        }

        return cells(row, col).get();
    }

    Box processing_region(const Box & raster_extent, const std::vector<Box> & component_boxes) {
        Box ret = Box::make_empty();
        for (const auto& box : component_boxes) {
            if (ret == raster_extent) {
                // No more expansion is possible
                return ret;
            }

            if (!box.intersects(raster_extent)) {
                continue;
            }

            Box isect = raster_extent.intersection(box);
            if (ret.empty()) {
                ret = isect;
            } else if (!ret.contains(isect)) {
                ret = ret.expand_to_include(isect);
            }
        }

        return ret;
    }

    static Grid<infinite_extent> get_geometry_grid(const Grid<bounded_extent> &raster_grid, GEOSContextHandle_t context, const GEOSGeometry* g) {
        if (GEOSisEmpty_r(context, g)) {
            throw std::invalid_argument("Can't get statistics for empty geometry");
        }

        Box region = processing_region(raster_grid.extent(), geos_get_component_boxes(context, g));

        if (!region.empty()) {
            return make_infinite(raster_grid.shrink_to_fit(region));
        } else {
            return Grid<infinite_extent>::make_empty();
        }
    }


    RasterCellIntersection::RasterCellIntersection(const Grid<bounded_extent> &raster_grid, GEOSContextHandle_t context, const GEOSGeometry *g)
        : m_geometry_grid{get_geometry_grid(raster_grid, context, g)},
          m_overlap_areas{std::make_unique<Matrix<float>>(m_geometry_grid.rows() - 2, m_geometry_grid.cols() - 2)}
    {
        if (!m_geometry_grid.empty())
            process(context, g);
    }

    void RasterCellIntersection::process(GEOSContextHandle_t context, const GEOSGeometry *g) {
        if (GEOSGeomTypeId_r(context, g) == GEOS_POLYGON) {
            process_ring(context, GEOSGetExteriorRing_r(context, g), true);

            for (int i = 0; i < GEOSGetNumInteriorRings_r(context, g); i++) {
                process_ring(context, GEOSGetInteriorRingN_r(context, g, i), false);
            }
        } else if (GEOSGeomTypeId_r(context, g) == GEOS_MULTIPOLYGON) {
            for (int i = 0; i < GEOSGetNumGeometries_r(context, g); i++) {
                process(context, GEOSGetGeometryN_r(context, g, i));
            }
        } else {
            throw std::invalid_argument("Unsupported geometry type.");
        }
    }

    static Grid<infinite_extent> get_ring_grid(GEOSContextHandle_t context, const GEOSGeometry* ls, const Grid<infinite_extent> & geometry_grid) {
        Box cropped_ring_extent = geometry_grid.extent().intersection(geos_get_box(context, ls));
        return geometry_grid.shrink_to_fit(cropped_ring_extent);
    }

    void RasterCellIntersection::process_ring(GEOSContextHandle_t context, const GEOSGeometry *ls, bool exterior_ring) {
        if (!geos_get_box(context, ls).intersects(m_geometry_grid.extent())) {
            return;
        }

        const GEOSCoordSequence *seq = GEOSGeom_getCoordSeq_r(context, ls);
        bool is_ccw = geos_is_ccw(context, seq);

        Grid<infinite_extent> ring_grid = get_ring_grid(context, ls, m_geometry_grid);

        size_t rows = ring_grid.rows();
        size_t cols = ring_grid.cols();

        Matrix<std::unique_ptr<Cell>> cells(rows, cols);

        std::deque<Coordinate> stk;
        {
            unsigned int npoints = geos_get_num_points(context, seq);

            for (unsigned int i = 0; i < npoints; i++) {
                double x, y;
                if (!GEOSCoordSeq_getX_r(context, seq, i, &x) || !GEOSCoordSeq_getY_r(context, seq, i, &y)) {
                    throw std::runtime_error("Error reading coordinates.");
                }

                if (is_ccw) {
                    stk.emplace_back(x, y);
                } else {
                    stk.emplace_front(x, y);
                }
            }
        }

        size_t row = ring_grid.get_row(stk.front().y);
        size_t col = ring_grid.get_column(stk.front().x);

        // A point lying exactly on a cell boundary could be considered to be
        // within either of the adjoining cells. This is fine unless the initial
        // segment of the ring is horizontal or vertical. In this case, we need
        // to nudge the point into the "inward" cell so that, if the initial segment
        // completely traverses the cell, we will have a traversal with a filled fraction
        // of 1.0 rather than a traversal with a filled fraction of 0.0. Leaving a traversal
        // with a filled fraction of 0.0 could allow a subsequent flood fill to penetrate
        // the interior of our polygon. If we are already at the edge of a grid, it's not possible
        // to nudge the point into the next cell, but we don't need to since there's no
        // possibility of a flood fill penetrating from this direction.
        SegmentOrientation iso = initial_segment_orientation(context, seq);
        if (iso != SegmentOrientation::ANGLED) {
            Box b = grid_cell(ring_grid, row, col);

            if (iso == SegmentOrientation::HORIZONTAL_RIGHT && stk.front().y == b.ymax && row > 0) {
                // Move up
                row--;
            }
            if (iso == SegmentOrientation::HORIZONTAL_LEFT && stk.front().y == b.ymin && (row + 1) < rows) {
                // Move down
                row++;
            }
            if (iso == SegmentOrientation::VERTICAL_DOWN && stk.front().x == b.xmax && (col + 1) < cols) {
                // Move right
                col++;
            }
            if (iso == SegmentOrientation::VERTICAL_UP && stk.front().x == b.xmin && col > 0) {
                // Move left
                col--;
            }
        }

        while (!stk.empty()) {
            Cell &cell = *get_cell(cells, ring_grid, row, col);

            while (!stk.empty()) {
                cell.take(stk.front());

                if (cell.last_traversal().exited()) {
                    // Only push our exit coordinate if it's not same as the
                    // coordinate we just took. This covers the case where
                    // the next coordinate in the stack falls exactly on
                    // the cell boundary.
                    const Coordinate &exc = cell.last_traversal().exit_coordinate();
                    if (exc != stk.front()) {
                        stk.emplace_front(exc.x, exc.y);
                    }
                    break;
                } else {
                    stk.pop_front();
                }
            }

            cell.force_exit();

            if (cell.last_traversal().exited()) {
                // When we start in the middle of a cell, we need to save the coordinates
                // from our incomplete traversal and reprocess them at the end of the line.
                // The effect is just to restructure the polygon so that the start/end
                // coordinate falls on a cell boundary.
                if (!cell.last_traversal().traversed()) {
                    for (const auto &coord : cell.last_traversal().coords()) {
                        stk.push_back(coord);
                    }
                }

                switch (cell.last_traversal().exit_side()) {
                    case Side::TOP:
                        row--;
                        break;
                    case Side::BOTTOM:
                        row++;
                        break;
                    case Side::LEFT:
                        col--;
                        break;
                    case Side::RIGHT:
                        col++;
                        break;
                    default:
                        throw std::runtime_error("Invalid traversal");
                }
            }
        }

        // Compute the fraction covered for all cells and assign it to
        // the area matrix
        // TODO avoid copying matrix when geometry has only one polygon, and polygon has only one ring
        Matrix<float> areas(rows - 2, cols - 2, fill_values<float>::FILLABLE);

        FloodFill ff(context, ls, make_finite(ring_grid));

        for (size_t i = 1; i <= areas.rows(); i++) {
            for (size_t j = 1; j <= areas.cols(); j++) {
                if (cells(i, j) != nullptr) {
                    // When we encounter a cell that has been processed (ie, it is not nullptr)
                    // but has zero covered fraction, we have no way to know if that cell is on
                    // the inside of the polygon. However, we do know that it is on the boundary
                    // of the polygon, and we cannot allow a flood fill to pass through it. So
                    // we set the value to UNKNOWN, which will both force a point-in-polygon
                    // test and block a flood fill.

                    auto frac = static_cast<float>(cells(i, j)->covered_fraction());
                    if (frac == 0) {
                        areas(i-1, j-1) = ff.cell_is_inside(i-1, j-1) ? fill_values<float>::INTERIOR : fill_values<float>::EXTERIOR;
                    } else {
                        areas(i-1, j-1) = frac;
                    }
                }
            }
        }

        ff.flood(areas);

        // Transfer these areas to our global set
        size_t i0 = ring_grid.row_offset(m_geometry_grid);
        size_t j0 = ring_grid.col_offset(m_geometry_grid);

        add_ring_areas(i0, j0, areas, exterior_ring);
    }

    void RasterCellIntersection::add_ring_areas(size_t i0, size_t j0, const Matrix<float> &areas, bool exterior_ring) {
        int factor = exterior_ring ? 1 : -1;

        for (size_t i = 0; i < areas.rows(); i++) {
            for (size_t j = 0; j < areas.cols(); j++) {
                m_overlap_areas->increment(i0 + i, j0 + j, factor * areas(i, j));
            }
        }
    }

}
