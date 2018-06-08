#include <geos_c.h>

#include "floodfill.h"
#include "geos_utils.h"
#include "raster_cell_intersection.h"
#include "segment_orientation.h"

static Cell* get_cell(Matrix<std::unique_ptr<Cell>> & cells, const Extent & ex, size_t row, size_t col) {
    //std::cout << " getting cell " << row << ", " << col << std::endl;

    if (cells(row, col) == nullptr) {
        cells(row, col) = ex.get_cell_ptr(row, col);
    }

    return cells(row, col).get();
}

RasterCellIntersection::RasterCellIntersection(const Extent & raster_extent, const GEOSGeometry* g) {
    if (GEOSisEmpty(g)) {
        throw std::invalid_argument("Can't get statistics for empty geometry");
    }

    m_geometry_extent = raster_extent.shrink_to_fit(geos_get_box(g));

    m_min_row = m_geometry_extent.row_offset();
    m_max_row = m_min_row + m_geometry_extent.rows();

    m_min_col = m_geometry_extent.col_offset();
    m_max_col = m_min_col + m_geometry_extent.cols();

    m_overlap_areas = std::make_unique<Matrix<float>>(m_max_row - m_min_row, m_max_col - m_min_col);

    process(g);
}

void RasterCellIntersection::process(const GEOSGeometry * g) {
    if (GEOSGeomTypeId(g) == GEOS_POLYGON) {
        process_ring(GEOSGetExteriorRing(g), true);

        for (int i = 0; i < GEOSGetNumInteriorRings(g); i++) {
            process_ring(GEOSGetInteriorRingN(g, i), false);
        }
    } else if (GEOSGeomTypeId(g) == GEOS_MULTIPOLYGON) {
        for (int i = 0; i < GEOSGetNumGeometries(g); i++) {
            process(GEOSGetGeometryN(g, i));
        }
    } else {
        throw std::invalid_argument("Unsupported geometry type.");
    }
}

void RasterCellIntersection::process_ring(const GEOSGeometry* ls, bool exterior_ring) {
    const GEOSCoordSequence* seq = GEOSGeom_getCoordSeq(ls);
    bool is_ccw = geos_is_ccw(seq);

    Extent ring_extent = m_geometry_extent.shrink_to_fit(geos_get_box(ls));

    size_t rows = ring_extent.rows();
    size_t cols = ring_extent.cols();

    // TODO avoid copying matrix when geometry has only one polygon, and polygon has only one ring
    Matrix<float> areas(rows, cols);
    Matrix<std::unique_ptr<Cell>> cells(rows, cols);

    std::deque<Coordinate> stk;
    {
        unsigned int npoints = geos_get_num_points(seq);

        for (unsigned int i = 0; i < npoints; i++) {
            double x, y;
            if (!GEOSCoordSeq_getX(seq, i, &x) || !GEOSCoordSeq_getY(seq, i, &y)) {
                throw std::runtime_error("Error reading coordinates.");
            }

            if (is_ccw) {
                stk.emplace_back(x, y);
            } else {
                stk.emplace_front(x, y);
            }
        }
    }

    size_t row = ring_extent.get_row(stk.front().y);
    size_t col = ring_extent.get_column(stk.front().x);

    // A point lying exactly on a cell boundary could be considered to be
    // within either of the adjoining cells. This is fine unless the initial
    // segment of the ring is horizontal or vertical. In this case, we need
    // to nudge the point into the "inward" cell so that, if the initial segment
    // completely traverses the cell, we will have a traversal with a filled fraction
    // of 1.0 rather than a traversal with a filled fraction of 0.0. Leaving a traversal
    // with a filled fraction of 0.0 could allow a subsequent flood fill to penetrate
    // the interior of our polygon. If we are already at the edge of a grid, it's not possibl
    // to nudge the point into the next cell, but we don't need to since there's no
    // possibility of a flood fill penetrating from this direction.
    SegmentOrientation iso = initial_segment_orientation(seq);
    if (iso != SegmentOrientation::ANGLED) {
        auto cell = ring_extent.get_cell_ptr(row, col);
        Box b{cell->box()};

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

    while(!stk.empty()) {
        Cell& cell = *get_cell(cells, ring_extent, row, col);

        while(!stk.empty()) {
            cell.take(stk.front());

            if (cell.last_traversal().exited()) {
                // Only push our exit coordinate if it's not same as the
                // coordinate we just took. This covers the case where
                // the next coordinate in the stack falls exactly on
                // the cell boundary.
                const Coordinate& exc = cell.last_traversal().exit_coordinate();
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
                for (const auto& coord : cell.last_traversal().coords()) {
                    stk.push_back(coord);
                }
            }

            switch(cell.last_traversal().exit_side()) {
                case Side::TOP:    row--; break;
                case Side::BOTTOM: row++; break;
                case Side::LEFT:   col--; break;
                case Side::RIGHT:  col++; break;
                default:
                    throw std::runtime_error("Invalid traversal");
            }
        }
    }

    // Compute the fraction covered for all cells and assign it to
    // the area matrix
    for (size_t i = 0; i < cells.rows(); i++) {
        for (size_t j = 0; j < cells.cols(); j++) {
            if (cells(i, j) != nullptr) {
                areas(i, j) = (float) cells(i, j)->covered_fraction();
            }
        }
    }

    FloodFill ff(ls, ring_extent);
    ff.flood(areas);

    // Transfer these areas to our global set
    size_t i0 = ring_extent.row_offset();
    size_t j0 = ring_extent.col_offset();

    add_ring_areas(i0, j0, areas, exterior_ring);
}

void RasterCellIntersection::add_ring_areas(size_t i0, size_t j0, const Matrix<float> & areas, bool exterior_ring) {
    int factor = exterior_ring ? 1 : -1;

    for (size_t i = 0; i < areas.rows(); i++) {
        for (size_t j = 0; j < areas.cols(); j++) {
            m_overlap_areas->increment(i0 + i, j0+ j, factor*areas(i, j));
        }
    }
}
