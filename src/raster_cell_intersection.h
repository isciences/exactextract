#ifndef RASTER_CELL_INTERSECTION_H
#define RASTER_CELL_INTERSECTION_H

#include <memory>

#include <geos_c.h>

#include "extent.h"
#include "matrix.h"

class RasterCellIntersection {

    public:
        RasterCellIntersection(const Extent & raster_extent, const GEOSGeometry* g);

        size_t min_row() const { return m_min_row; }
        size_t min_col() const { return m_min_col; }
        size_t max_row() const { return m_max_row; }
        size_t max_col() const { return m_max_col; }
        size_t rows() const { return max_row() - min_row(); }
        size_t cols() const { return max_col() - min_col(); }

        float get(size_t i, size_t j) const { return m_overlap_areas->operator()(i - m_min_row, j - m_min_col); }
        float get_local(size_t i, size_t j) const { return m_overlap_areas->operator()(i, j); }

        const Matrix<float>& overlap_areas() const { return *m_overlap_areas; } 

    private:
        size_t m_min_row;
        size_t m_min_col;
        size_t m_max_row;
        size_t m_max_col;

        Extent m_geometry_extent;

        void process(const GEOSGeometry * g);
        void process_ring(const GEOSGeometry * ls, bool exterior_ring);
        void add_ring_areas(size_t i0, size_t j0, const Matrix<float> & areas, bool exterior_ring);

        std::unique_ptr<Matrix<float>> m_overlap_areas;

};

#endif