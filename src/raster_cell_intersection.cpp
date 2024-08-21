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

#include <stdexcept>

#include <geos_c.h>

#include "cell.h"
#include "floodfill.h"
#include "geos_utils.h"
#include "measures.h"
#include "raster_cell_intersection.h"

namespace exactextract {

Raster<float>
raster_cell_intersection(const Grid<bounded_extent>& raster_grid, GEOSContextHandle_t context, const GEOSGeometry* g)
{
    RasterCellIntersection rci(raster_grid, context, g);

    return { std::move(const_cast<Matrix<float>&>(rci.results())),
             make_finite(rci.m_geometry_grid) };
}

Raster<float>
raster_cell_intersection(const Grid<bounded_extent>& raster_grid, const Box& box)
{
    RasterCellIntersection rci(raster_grid, box);

    return { std::move(const_cast<Matrix<float>&>(rci.results())),
             make_finite(rci.m_geometry_grid) };
}

static Cell*
get_cell(Matrix<std::unique_ptr<Cell>>& cells, const Grid<infinite_extent>& ex, size_t row, size_t col)
{
    if (cells(row, col) == nullptr) {
        cells(row, col) = std::make_unique<Cell>(grid_cell(ex, row, col));
    }

    return cells(row, col).get();
}

Box
processing_region(const Box& raster_extent, const std::vector<Box>& component_boxes)
{
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

static Grid<infinite_extent>
get_geometry_grid(const Grid<bounded_extent>& raster_grid, GEOSContextHandle_t context, const GEOSGeometry* g)
{
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

static Grid<infinite_extent>
get_geometry_grid(const Grid<bounded_extent>& raster_grid, const Box& box)
{
    auto region = box.intersection(raster_grid.extent());

    if (!region.empty()) {
        return make_infinite(raster_grid.shrink_to_fit(region));
    } else {
        return Grid<infinite_extent>::make_empty();
    }
}

RasterCellIntersection::RasterCellIntersection(const Grid<bounded_extent>& raster_grid, GEOSContextHandle_t context, const GEOSGeometry* g)
  : m_geometry_grid{ get_geometry_grid(raster_grid, context, g) }
  , m_results{ std::make_unique<Matrix<float>>(m_geometry_grid.rows() - 2, m_geometry_grid.cols() - 2) }
  , m_first_geom{ true }
  , m_areal{ false }
{
    if (GEOSGeom_getDimensions_r(context, g) == 0) {
        throw std::invalid_argument("Unsupported geometry type.");
    }

    if (!m_geometry_grid.empty()) {
        process(context, g);
    }
}

RasterCellIntersection::RasterCellIntersection(const Grid<bounded_extent>& raster_grid, const Box& box)
  : m_geometry_grid{ get_geometry_grid(raster_grid, box) }
  , m_results{ std::make_unique<Matrix<float>>(m_geometry_grid.rows() - 2, m_geometry_grid.cols() - 2) }
{
    if (!m_geometry_grid.empty()) {
        process_rectangular_ring(box, true);
    }
}

void
RasterCellIntersection::process(GEOSContextHandle_t context, const GEOSGeometry* g)
{
    auto type = GEOSGeomTypeId_r(context, g);

    if (type == GEOS_POLYGON) {
        set_areal(true);

        process_line(context, GEOSGetExteriorRing_r(context, g), true);
        for (int i = 0; i < GEOSGetNumInteriorRings_r(context, g); i++) {
            process_line(context, GEOSGetInteriorRingN_r(context, g, i), false);
        }
    } else if (type == GEOS_LINESTRING || type == GEOS_LINEARRING) {
        set_areal(false);

        process_line(context, g, true);
    } else if (type == GEOS_GEOMETRYCOLLECTION || type == GEOS_MULTILINESTRING || type == GEOS_MULTIPOLYGON) {
        for (int i = 0; i < GEOSGetNumGeometries_r(context, g); i++) {
            process(context, GEOSGetGeometryN_r(context, g, i));
        }
    } else {
        throw std::invalid_argument("Unsupported geometry type.");
    }
}

static Grid<infinite_extent>
get_box_grid(const Box& box, const Grid<infinite_extent>& geometry_grid)
{
    Box cropped_ring_extent = geometry_grid.extent().intersection(box);
    return geometry_grid.shrink_to_fit(cropped_ring_extent);
}

static Grid<infinite_extent>
get_ring_grid(GEOSContextHandle_t context, const GEOSGeometry* ls, const Grid<infinite_extent>& geometry_grid)
{
    return get_box_grid(geos_get_box(context, ls), geometry_grid);
}

#if 0
    static Grid<infinite_extent> get_ring_grid(GEOSContextHandle_t context, const GEOSGeometry* ls, const Grid<infinite_extent> & geometry_grid) {
        Box cropped_ring_extent = geometry_grid.extent().intersection(geos_get_box(context, ls));
        return geometry_grid.shrink_to_fit(cropped_ring_extent);
    }
#endif

void
RasterCellIntersection::process_rectangular_ring(const Box& box, bool exterior_ring)
{
    if (!box.intersects(m_geometry_grid.extent())) {
        return;
    }

    auto ring_grid = get_box_grid(box, m_geometry_grid);

    auto row_min = ring_grid.get_row(box.ymax);
    auto row_max = ring_grid.get_row(box.ymin);
    auto col_min = ring_grid.get_column(box.xmin);
    auto col_max = ring_grid.get_column(box.xmax);

    Matrix<float> areas(ring_grid.rows() - 2, ring_grid.cols() - 2);

    // upper-left
    if (row_min > 0 && col_min > 0) {
        auto ul = grid_cell(ring_grid, row_min, col_min);
        areas(row_min - 1, col_min - 1) = ul.intersection(box).area() / ul.area();
    }

    // upper-right
    if (row_min > 0 && col_max < ring_grid.cols() - 1) {
        auto ur = grid_cell(ring_grid, row_min, col_max);
        auto frac = ur.intersection(box).area() / ur.area();
        areas(row_min - 1, col_max - 1) = frac;
    }

    // lower-left
    if (row_max < ring_grid.rows() - 1 && col_min > 0) {
        auto ll = grid_cell(ring_grid, row_max, col_min);
        areas(row_max - 1, col_min - 1) = ll.intersection(box).area() / ll.area();
    }

    // lower-right
    if (row_max < ring_grid.rows() - 1 && col_max < ring_grid.cols() - 1) {
        auto lr = grid_cell(ring_grid, row_max, col_max);
        areas(row_max - 1, col_max - 1) = lr.intersection(box).area() / lr.area();
    }

    // left
    if (col_min > 0) {
        auto left = grid_cell(ring_grid, row_min + 1, col_min);
        auto frac = left.intersection(box).area() / left.area();
        for (size_t row = row_min + 1; row < row_max; row++) {
            areas(row - 1, col_min - 1) = frac;
        }
    }

    // right
    if (col_max < ring_grid.cols() - 1) {
        auto right = grid_cell(ring_grid, row_min + 1, col_max);
        auto frac = right.intersection(box).area() / right.area();
        for (size_t row = row_min + 1; row < row_max; row++) {
            areas(row - 1, col_max - 1) = frac;
        }
    }

    // top
    if (row_min > 0) {
        auto top = grid_cell(ring_grid, row_min, col_min + 1);
        auto frac = top.intersection(box).area() / top.area();
        for (size_t col = col_min + 1; col < col_max; col++) {
            areas(row_min - 1, col - 1) = frac;
        }
    }

    // bottom
    if (row_max < ring_grid.rows() - 1) {
        auto bottom = grid_cell(ring_grid, row_max, col_min + 1);
        auto frac = bottom.intersection(box).area() / bottom.area();
        for (size_t col = col_min + 1; col < col_max; col++) {
            areas(row_max - 1, col - 1) = frac;
        }
    }

    // interior
    for (size_t row = row_min + 1; row < row_max; row++) {
        for (size_t col = col_min + 1; col < col_max; col++) {
            areas(row - 1, col - 1) = 1.0f;
        }
    }

    // Transfer these areas to our global set
    size_t i0 = ring_grid.row_offset(m_geometry_grid);
    size_t j0 = ring_grid.col_offset(m_geometry_grid);

    add_ring_results(i0, j0, areas, exterior_ring);
}

void
RasterCellIntersection::set_areal(bool areal)
{
    if (m_first_geom) {
        m_first_geom = false;
        m_areal = areal;
    } else {
        if (m_areal != areal) {
            throw std::runtime_error("Mixed-type geometries not supported.");
        }
    }
}

Matrix<float>
RasterCellIntersection::collect_areas(const Matrix<std::unique_ptr<Cell>>& cells,
                                      const Grid<bounded_extent>& finite_ring_grid,
                                      GEOSContextHandle_t context,
                                      const GEOSGeometry* ls)
{

    // Create a matrix of areas using the FILLABLE placeholder value, which means that
    // a known state (interior/exterior) can be propated from an adjacent cell.
    Matrix<float> areas(cells.rows() - 2,
                        cells.cols() - 2,
                        fill_values<float>::FILLABLE);

    FloodFill ff(context, ls, finite_ring_grid);

    // Mark all cells that have been traversed as being either INTERIOR or EXTERIOR.
    for (size_t i = 1; i <= areas.rows(); i++) {
        for (size_t j = 1; j <= areas.cols(); j++) {
            if (cells(i, j) != nullptr) {
                // When we encounter a cell that has been processed (ie, it is not nullptr)
                // but whose traversals contains no linear segments, we have no way to know
                // if it is inside of the polygon. So we perform point-in-polygon test and set
                // the covered fraction to 1.0 if needed.

                if (!cells(i, j)->determined()) {
                    areas(i - 1, j - 1) = ff.cell_is_inside(i - 1, j - 1) ? fill_values<float>::INTERIOR : fill_values<float>::EXTERIOR;
                } else {
                    areas(i - 1, j - 1) = cells(i, j)->covered_fraction(context);
                }
            }
        }
    }

    // Propagate INTERIOR and EXTERIOR values to all remaining FILLABLE cells
    ff.flood(areas);

    return areas;
}

Matrix<float>
collect_lengths(const Matrix<std::unique_ptr<Cell>>& cells)
{
    Matrix<float> lengths(cells.rows() - 2,
                          cells.cols() - 2,
                          fill_values<float>::EXTERIOR);

    for (size_t i = 1; i <= lengths.rows(); i++) {
        for (size_t j = 1; j <= lengths.cols(); j++) {
            if (cells(i, j) != nullptr) {
                lengths(i - 1, j - 1) = static_cast<float>(cells(i, j)->traversal_length());
            }
        }
    }

    return lengths;
}

void
traverse_cells(Matrix<std::unique_ptr<Cell>>& cells, std::vector<Coordinate>& coords, const Grid<infinite_extent>& ring_grid, bool areal)
{
    size_t pos = 0;
    size_t row = ring_grid.get_row(coords.front().y);
    size_t col = ring_grid.get_column(coords.front().x);
    const Coordinate* last_exit = nullptr;

    while (pos < coords.size()) {
        Cell& cell = *get_cell(cells, ring_grid, row, col);

        while (pos < coords.size()) {
            const Coordinate* next_coord = last_exit ? last_exit : &coords[pos];
            const Coordinate* prev_coord = pos > 0 ? &coords[pos - 1] : nullptr;

            cell.take(*next_coord, prev_coord);

            if (cell.last_traversal().exited()) {
                // Only push our exit coordinate if it's not same as the
                // coordinate we just took. This covers the case where
                // the next coordinate in the stack falls exactly on
                // the cell boundary.
                const Coordinate& exc = cell.last_traversal().exit_coordinate();
                if (exc != *next_coord) {
                    last_exit = &exc;
                }
                break;
            } else {
                if (last_exit) {
                    last_exit = nullptr;
                } else {
                    pos++;
                }
            }
        }

        cell.force_exit();

        if (cell.last_traversal().exited()) {
            // When we start in the middle of a cell, for a polygon (areal) calculation,
            // we need to save the coordinates from our incomplete traversal and reprocess
            // them at the end of the line. The effect is just to restructure the polygon
            // so that the start/end coordinate falls on a cell boundary.
            if (areal && !cell.last_traversal().traversed()) {
                for (const auto& coord : cell.last_traversal().coords()) {
                    coords.push_back(coord);
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
}

void
RasterCellIntersection::process_line(GEOSContextHandle_t context, const GEOSGeometry* ls, bool exterior_ring)
{
    auto geom_box = geos_get_box(context, ls);

    if (!geom_box.intersects(m_geometry_grid.extent())) {
        return;
    }

    const GEOSCoordSequence* seq = GEOSGeom_getCoordSeq_r(context, ls);
    auto coords = read(context, seq);

    if (m_areal) {
        if (coords.size() < 4) {
            // Component cannot have any area, just skip it.
            return;
        }

        if (coords.size() == 5 && area(coords) == geom_box.area()) {
            process_rectangular_ring(geom_box, exterior_ring);
            return;
        }
    }

    Grid<infinite_extent> ring_grid = get_ring_grid(context, ls, m_geometry_grid);

    size_t rows = ring_grid.rows();
    size_t cols = ring_grid.cols();

    // Short circuit for small geometries that are entirely contained within a single grid cell.
    if (rows == (1 + 2 * infinite_extent::padding) &&
        cols == (1 + 2 * infinite_extent::padding) &&
        grid_cell(ring_grid, 1, 1).contains(geom_box)) {

        size_t i0 = ring_grid.row_offset(m_geometry_grid);
        size_t j0 = ring_grid.col_offset(m_geometry_grid);

        if (m_areal) {
            auto ring_area = area(coords) / grid_cell(ring_grid, 1, 1).area();

            if (exterior_ring) {
                m_results->increment(i0, j0, ring_area);
            } else {
                m_results->increment(i0, j0, -1 * ring_area);
            }
        } else {
            m_results->increment(i0, j0, length(coords));
        }

        return;
    }

    if (m_areal && !geos_is_ccw(context, seq)) {
        std::reverse(coords.begin(), coords.end());
    }

    Matrix<std::unique_ptr<Cell>> cells(ring_grid.rows(), ring_grid.cols());
    traverse_cells(cells, coords, ring_grid, m_areal);

    // Compute the fraction covered for all cells and assign it to
    // the area matrix
    // TODO avoid copying matrix when geometry has only one polygon, and polygon has only one ring
    Matrix<float> areas = m_areal ? collect_areas(cells, make_finite(ring_grid), context, ls) : collect_lengths(cells);

    // Transfer these areas to our global set
    size_t i0 = ring_grid.row_offset(m_geometry_grid);
    size_t j0 = ring_grid.col_offset(m_geometry_grid);

    add_ring_results(i0, j0, areas, exterior_ring || !m_areal);
}

void
RasterCellIntersection::add_ring_results(size_t i0, size_t j0, const Matrix<float>& areas, bool exterior_ring)
{
    int factor = exterior_ring ? 1 : -1;

    for (size_t i = 0; i < areas.rows(); i++) {
        for (size_t j = 0; j < areas.cols(); j++) {
            m_results->increment(i0 + i, j0 + j, factor * areas(i, j));
        }
    }
}

void
traverse_ring(Matrix<std::unique_ptr<Cell>>& cells, const Grid<infinite_extent>& grid, GEOSContextHandle_t context, const GEOSGeometry* g, bool want_ccw)
{
    const GEOSCoordSequence* seq = GEOSGeom_getCoordSeq_r(context, g);
    auto coords = read(context, seq);

    bool is_ccw = geos_is_ccw(context, seq);
    if (want_ccw != is_ccw) {
        std::reverse(coords.begin(), coords.end());
    }

    traverse_cells(cells, coords, grid, true);
}

geom_ptr_r
RasterCellIntersection::subdivide_polygon(const Grid<bounded_extent>& p_grid, GEOSContextHandle_t context, const GEOSGeometry* g)
{
    Grid<infinite_extent> grid = make_infinite(p_grid);
    Matrix<std::unique_ptr<Cell>> cells(grid.rows(), grid.cols());

    int ngeoms = GEOSGetNumGeometries_r(context, g);
    for (int i = 0; i < ngeoms; i++) {
        const GEOSGeometry* gi = GEOSGetGeometryN_r(context, g, i);
        const GEOSGeometry* shell = GEOSGetExteriorRing_r(context, gi);
        traverse_ring(cells, grid, context, shell, true);

        int nrings = GEOSGetNumInteriorRings_r(context, gi);
        for (int j = 0; j < nrings; j++) {
            traverse_ring(cells, grid, context, GEOSGetInteriorRingN_r(context, gi, j), false);
        }
    }

    auto areas = collect_areas(cells, p_grid, context, g);

    std::vector<GEOSGeometry*> geoms;

    // consider including first and last row/column
    for (std::size_t i = 1; i < cells.rows() - 1; i++) {
        for (std::size_t j = 1; j < cells.cols() - 1; j++) {
            if (cells(i, j) != nullptr) {
                if (areas(i - 1, j - 1) == fill_values<float>::INTERIOR) {
                    // Cell is completely covered by polygon
                    geoms.push_back(geos_make_box_polygon(context, cells(i, j)->box()).release());
                } else if (areas(i - 1, j - 1) != fill_values<float>::EXTERIOR) {
                    // Cell is partly covered by polygon
                    geoms.push_back(cells(i, j)->covered_polygons(context).release());
                }
            } else if (areas(i - 1, j - 1) == fill_values<float>::INTERIOR) {
                // Cell is entirely covered by polygon
                geoms.push_back(geos_make_box_polygon(context, grid_cell(grid, i, j)).release());
            }
        }
    }

    return geos_ptr(context,
                    GEOSGeom_createCollection_r(context, GEOS_GEOMETRYCOLLECTION, geoms.data(), geoms.size()));
}

}
