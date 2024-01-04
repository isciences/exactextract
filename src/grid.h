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

#ifndef EXACTEXTRACT_GRID_H
#define EXACTEXTRACT_GRID_H

#include <numeric>
#include <stdexcept>
#include <vector>

#include "box.h"

namespace exactextract {
struct infinite_extent
{
    static const size_t padding = 1;
};

struct bounded_extent
{
    static const size_t padding = 0;
};

static inline bool
is_integral(double d, double tol)
{
    return std::abs(d - std::round(d)) <= tol;
}

template<typename extent_tag>
class Grid
{

  public:
    Grid(const Box& extent, double dx, double dy)
      : m_extent{ extent }
      , m_dx{ dx }
      , m_dy{ dy }
      , m_num_rows{ 2 * extent_tag::padding + (extent.ymax > extent.ymin ? static_cast<size_t>(std::round((extent.ymax - extent.ymin) / dy)) : 0) }
      , m_num_cols{ 2 * extent_tag::padding + (extent.xmax > extent.xmin ? static_cast<size_t>(std::round((extent.xmax - extent.xmin) / dx)) : 0) }
    {
    }

    static Grid make_empty()
    {
        return Grid({ 0, 0, 0, 0 }, 0, 0);
    }

    size_t get_column(double x) const
    {
        if (extent_tag::padding) {
            if (x < m_extent.xmin)
                return 0;
            if (x > m_extent.xmax)
                return m_num_cols - 1;
            if (x == m_extent.xmax) // special case, returning the cell for which xmax is the right
                return m_num_cols - 2;
        } else {
            if (x < m_extent.xmin || x > m_extent.xmax)
                throw std::out_of_range("x");

            if (x == m_extent.xmax)
                return m_num_cols - 1;
        }

        // Since we've already range-checked our x value, make sure that
        // the computed column index is no greater than the column index
        // associated with xmax.
        return std::min(
          extent_tag::padding + static_cast<size_t>(std::floor((x - m_extent.xmin) / m_dx)),
          get_column(m_extent.xmax));
    }

    size_t get_row(double y) const
    {
        if (extent_tag::padding) {
            if (y > m_extent.ymax)
                return 0;
            if (y < m_extent.ymin)
                return m_num_rows - 1;
            if (y == m_extent.ymin) // special case, returning the cell for which ymin is the bottom
                return m_num_rows - 2;
        } else {
            if (y < m_extent.ymin || y > m_extent.ymax)
                throw std::out_of_range("y");

            if (y == m_extent.ymin)
                return m_num_rows - 1;
        }

        // Since we've already range-checked our y value, make sure that
        // the computed row index is no greater than the column index
        // associated with ymin.
        return std::min(
          extent_tag::padding + static_cast<size_t>(std::floor((m_extent.ymax - y) / m_dy)),
          get_row(m_extent.ymin));
    }

    std::size_t get_cell(double x, double y) const
    {
        return get_row(y) * cols() + get_column(x);
    }

    bool empty() const { return m_num_rows <= 2 * extent_tag::padding && m_num_cols <= 2 * extent_tag::padding; }

    size_t rows() const { return m_num_rows; }

    size_t cols() const { return m_num_cols; }

    size_t size() const { return rows() * cols(); }

    double xmin() const { return m_extent.xmin; }

    double xmax() const { return m_extent.xmax; }

    double ymin() const { return m_extent.ymin; }

    double ymax() const { return m_extent.ymax; }

    double dx() const { return m_dx; }

    double dy() const { return m_dy; }

    const Box& extent() const { return m_extent; }

    size_t row_offset(const Grid& other) const { return static_cast<size_t>(std::round(std::abs(other.m_extent.ymax - m_extent.ymax) / m_dy)); }

    size_t col_offset(const Grid& other) const { return static_cast<size_t>(std::round(std::abs(m_extent.xmin - other.m_extent.xmin) / m_dx)); }

    double x_for_col(size_t col) const { return m_extent.xmin + ((col - extent_tag::padding) + 0.5) * m_dx; }

    double y_for_row(size_t row) const { return m_extent.ymax - ((row - extent_tag::padding) + 0.5) * m_dy; }

    Grid<extent_tag> crop(const Box& b) const
    {
        if (extent().intersects(b)) {
            return shrink_to_fit(b.intersection(extent()));
        } else {
            return make_empty();
        }
    }

    Grid<extent_tag> shrink_to_fit(const Box& b) const
    {
        if (b.empty()) {
            return make_empty();
        }

        if (b.xmin < m_extent.xmin || b.ymin < m_extent.ymin || b.xmax > m_extent.xmax || b.ymax > m_extent.ymax) {
            throw std::range_error("Cannot shrink extent to bounds larger than original.");
        }

        size_t col0 = get_column(b.xmin);
        size_t row1 = get_row(b.ymax);

        // Shrink xmin and ymax to fit the upper-left corner of the supplied extent
        double snapped_xmin = m_extent.xmin + (col0 - extent_tag::padding) * m_dx;
        double snapped_ymax = m_extent.ymax - (row1 - extent_tag::padding) * m_dy;

        // Make sure x0 and y1 are within the reduced extent. Because of
        // floating point round-off errors, this is not always the case.
        if (b.xmin < snapped_xmin) {
            snapped_xmin -= m_dx;
            col0--;
        }

        if (b.ymax > snapped_ymax) {
            snapped_ymax += m_dy;
            row1--;
        }

        size_t col1 = get_column(b.xmax);
        size_t row0 = get_row(b.ymin);

        size_t num_rows = 1 + (row0 - row1);
        size_t num_cols = 1 + (col1 - col0);

        // If xmax or ymin falls cleanly on a cell boundary, we don't
        // need as many rows or columns as we otherwise would, because
        // we assume that the rightmost cell of the grid is a closed
        // interval in x, and the lowermost cell of the grid is a
        // closed interval in y.
        if (num_rows > 2 && (snapped_ymax - (num_rows - 1) * m_dy <= b.ymin)) {
            num_rows--;
        }
        if (num_cols > 2 && (snapped_xmin + (num_cols - 1) * m_dx >= b.xmax)) {
            num_cols--;
        }

        // Perform offsets relative to the new xmin, ymax origin
        // points. If this is not done, then floating point roundoff
        // error can cause progressive shrink() calls with the same
        // inputs to produce different results.
        Box reduced_box = {
            snapped_xmin,
            std::min(snapped_ymax - num_rows * m_dy, b.ymin),
            std::max(snapped_xmin + num_cols * m_dx, b.xmax),
            snapped_ymax
        };

        // Fudge computed xmax and ymin, if needed, to prevent extent
        // from growing during a shrink operation.
        if (reduced_box.xmax > m_extent.xmax) {
            if (std::round((reduced_box.xmax - reduced_box.xmin) / m_dx) ==
                std::round((m_extent.xmax - reduced_box.xmin) / m_dx)) {
                reduced_box.xmax = m_extent.xmax;
            } else {
                throw std::runtime_error("Shrink operation failed.");
            }
        }
        if (reduced_box.ymin < m_extent.ymin) {
            if (std::round((reduced_box.ymax - reduced_box.ymin) / m_dy) ==
                std::round((reduced_box.ymax - m_extent.ymin) / m_dy)) {
                reduced_box.ymin = m_extent.ymin;
            } else {
                throw std::runtime_error("Shrink operation failed.");
            }
        }

        Grid<extent_tag> reduced{ reduced_box, m_dx, m_dy };

        if (b.xmin < reduced.xmin() || b.ymin < reduced.ymin() || b.xmax > reduced.xmax() || b.ymax > reduced.ymax()) {
            throw std::runtime_error("Shrink operation failed.");
        }

        return reduced;
    }

    template<typename extent_tag2>
    bool compatible_with(const Grid<extent_tag2>& b, double compatability_tol) const
    {
        // Define a tolerance for grid compatibility, to be used in the following ways:
        // Grid resolutions must be integer multiples of each other within a factor of 'compatibility_tol'
        // Grid origin points must differ by an integer multiple of the smaller of the two grid resolutions,
        // within a factor of 'compatibility_tol'.

        if (empty() || b.empty()) {
            return true;
        }

        double xtol = std::min(m_dx, b.m_dx) * compatability_tol;
        double ytol = std::min(m_dy, b.m_dy) * compatability_tol;

        // Check x-resolution compatibility
        if (!is_integral(std::max(m_dx, b.m_dx) / std::min(m_dx, b.m_dx), xtol)) {
            return false;
        }

        // Check y-resolution compatibility
        if (!is_integral(std::max(m_dy, b.m_dy) / std::min(m_dy, b.m_dy), ytol)) {
            return false;
        }

        // Check left-hand boundary compatibility
        if (!is_integral(std::abs(b.m_extent.xmin - m_extent.xmin) / std::min(m_dx, b.m_dx), xtol)) {
            return false;
        }

        // Check upper boundary compatibility
        if (!is_integral(std::abs(b.m_extent.ymax - m_extent.ymax) / std::min(m_dy, b.m_dy), ytol)) {
            return false;
        }

        return true;
    }

    template<typename extent_tag2>
    Grid<extent_tag> common_grid(const Grid<extent_tag2>& b, double tol = 1e-6) const
    {
        if (!compatible_with(b, tol)) {
            throw std::runtime_error("Incompatible extents.");
        }

        if (b.empty()) {
            return *this;
        }

        const double common_dx = std::min(m_dx, b.m_dx);
        const double common_dy = std::min(m_dy, b.m_dy);

        const double common_xmin = std::min(m_extent.xmin, b.m_extent.xmin);
        const double common_ymax = std::max(m_extent.ymax, b.m_extent.ymax);

        double common_xmax = std::max(m_extent.xmax, b.m_extent.xmax);
        double common_ymin = std::min(m_extent.ymin, b.m_extent.ymin);

        const long nx = static_cast<long>(std::round((common_xmax - common_xmin) / common_dx));
        const long ny = static_cast<long>(std::round((common_ymax - common_ymin) / common_dy));

        common_xmax = std::max(common_xmax, common_xmin + nx * common_dx);
        common_ymin = std::min(common_ymin, common_ymax - ny * common_dy);

        return { { common_xmin, common_ymin, common_xmax, common_ymax }, common_dx, common_dy };
    }

    template<typename extent_tag2>
    Grid<extent_tag> overlapping_grid(const Grid<extent_tag2>& b, double tol = 1e-6) const
    {
        if (!compatible_with(b, tol)) {
            throw std::runtime_error("Incompatible extents.");
        }

        if (empty() || b.empty()) {
            return make_empty();
        }

        const double common_dx = std::min(m_dx, b.m_dx);
        const double common_dy = std::min(m_dy, b.m_dy);

        const double common_xmin = std::max(m_extent.xmin, b.m_extent.xmin);
        const double common_ymax = std::min(m_extent.ymax, b.m_extent.ymax);

        double common_xmax = std::min(m_extent.xmax, b.m_extent.xmax);
        double common_ymin = std::max(m_extent.ymin, b.m_extent.ymin);

        const long nx = static_cast<long>(std::round((common_xmax - common_xmin) / common_dx));
        const long ny = static_cast<long>(std::round((common_ymax - common_ymin) / common_dy));

        common_xmax = std::max(common_xmax, common_xmin + nx * common_dx);
        common_ymin = std::min(common_ymin, common_ymax - ny * common_dy);

        return { { common_xmin, common_ymin, common_xmax, common_ymax }, common_dx, common_dy };
    }

    bool operator==(const Grid<extent_tag>& b) const
    {
        return m_extent == b.m_extent &&
               m_dx == b.m_dx &&
               m_dy == b.m_dy;
    }

    bool operator!=(const Grid<extent_tag>& b) const
    {
        return !(*this == b);
    }

  private:
    Box m_extent;

    double m_dx;
    double m_dy;

    size_t m_num_rows;
    size_t m_num_cols;
};

Box
grid_cell(const Grid<bounded_extent>& grid, size_t row, size_t col);
Box
grid_cell(const Grid<infinite_extent>& grid, size_t row, size_t col);

Grid<infinite_extent>
make_infinite(const Grid<bounded_extent>& grid);
Grid<bounded_extent>
make_finite(const Grid<infinite_extent>& grid);

std::vector<Grid<bounded_extent>>
subdivide(const Grid<bounded_extent>& grid, size_t max_size);

template<typename T>
Grid<bounded_extent>
common_grid(T begin, T end)
{
    if (begin == end) {
        return Grid<bounded_extent>::make_empty();
    } else if (std::next(begin) == end) {
        return (*begin)->grid();
    }
    return std::accumulate(
      std::next(begin),
      end,
      (*begin)->grid(),
      [](auto& acc, auto& op) {
          return acc.common_grid(op->grid());
      });
}

}

#endif // EXACTEXTRACT_INFINITEGRID_H
