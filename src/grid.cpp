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

#include "grid.h"

#include <cmath>
#include <memory>
#include <stdexcept>

namespace exactextract {

    static double tol = 5e-14;

    static inline bool lt(double a, double b) {
        return (b - a) > tol;
    }

    static inline bool gt(double a, double b) {
        return lt(b, a);
    }

    static inline bool is_integral(double d) {
        return d == std::floor(d);
    }

    Grid::Grid(double xmin, double ymin, double xmax, double ymax, double dx, double dy) :
            xmin{xmin},
            ymin{ymin},
            xmax{xmax},
            ymax{ymax},
            dx{dx},
            dy{dy} {
        // Compute and store the number of rows and columns. Do this so that we can be sure
        // to get the correct number. Because we believe xmax and ymax are exact multiples
        // of dx, we can find the number of cells using round instead of floor.
        // The floor function , which we use for the general case in getCol(x) and getRow(y),
        // will give incorrect results in some cases.
        // For example, the following floor calculations return the same result,
        // when the column numbers should clearly be different:
        // floor((16.2 - 8.5) / 0.1) --> 76
        // floor((16.1 - 8.5) / 0.1) --> 76
        m_num_cols = (size_t) std::round((xmax - xmin) / dx);
        m_num_rows = (size_t) std::round((ymax - ymin) / dy);
    }

    size_t Grid::get_column(double x) const {
        if (lt(x, xmin) || gt(x, xmax)) {
            throw std::out_of_range("x");
        }

        // special case for xmax, returning the cell for which xmax is the
        // right-hand side
        if (xmax - x <= tol) {
            return m_num_cols - 1;
        }

        return (size_t) std::floor((x - xmin) / dx);
    }

    size_t Grid::get_row(double y) const {
        if (lt(y, ymin) || gt(y,  ymax))
            throw std::out_of_range("y");

        // special case for ymin, returning the cell for which ymin is the
        // bottom
        if (y - ymin <= tol)
            return m_num_rows - 1;

        return (size_t) std::floor((ymax - y) / dy);
    }

    Grid Grid::shrink_to_fit(const Box &b) const {
        return shrink_to_fit(b.xmin, b.ymin, b.xmax, b.ymax);
    }

    Grid Grid::shrink_to_fit(double x0, double y0, double x1, double y1) const {
        if (lt(x0, xmin) || lt(y0, ymin) || gt(x1, xmax) || gt(y1, ymax)) {
            throw std::range_error("Cannot shrink extent to bounds larger than original.");
        }

        size_t col0 = get_column(x0);
        size_t row1 = get_row(y1);

        // Shrink xmin and ymax to fit the upper-let corner of the supplied extent
        double snapped_xmin = xmin + col0 * dx;
        double snapped_ymax = ymax - row1 * dy;

        // Make sure x0 and y1 are within the reduced extent. Because of
        // floating point round-off errors, this is not always the case.
        if (x0 < snapped_xmin) {
            snapped_xmin -= dx;
            col0--;
        }

        if (y1 > snapped_ymax) {
            snapped_ymax += dy;
            row1--;
        }

        size_t col1 = get_column(x1);
        size_t row0 = get_row(y0);

        size_t numRows = 1 + (row0 - row1);
        size_t numCols = 1 + (col1 - col0);

        // Perform offsets relative to the new xmin, ymax origin
        // points. If this is not done, then floating point roundoff
        // error can cause progressive shrink() calls with the same
        // inputs to produce different results.
        Grid reduced(
                snapped_xmin,
                std::min(snapped_ymax - numRows * dy, y0),
                std::max(snapped_xmin + numCols * dx, x1),
                snapped_ymax,
                dx,
                dy);

        if (lt(x0, reduced.xmin) || lt(y0, reduced.ymin) || gt(x1, reduced.xmax) || gt(y1, reduced.ymax)) {
            throw std::runtime_error("Shrink operation failed.");
        }

        return reduced;
    }

    std::unique_ptr<Cell> Grid::get_cell_ptr(size_t row, size_t col) const {
        // The ternary clauses below are used to make sure that the cells along
        // the right and bottom edges of our grid are slightly larger than dx,dy
        // if needed to make sure that we capture our whole extent. This is necessary
        // because xmin + nx*dx may be less than xmax because of floating point
        // errors.
        return std::make_unique<Cell>(
                xmin + col * dx,
                row == (m_num_rows - 1) ? ymin : (ymax - (row + 1) * dy),
                col == (m_num_cols - 1) ? xmax : (xmin + (col + 1) * dx),
                ymax - row * dy

        );
    }

    bool Grid::compatible_with(const exactextract::Grid &b) const {
        // Check x-resolution compatibility
        if (!is_integral(std::max(dx, b.dx) / std::min(dx, b.dx))) {
            return false;
        }

        // Check y-resolution compatibility
        if (!is_integral(std::max(dy, b.dy) / std::min(dy, b.dy))) {
            return false;
        }

        // Check left-hand boundary compatibility
        if (!is_integral(std::abs(b.xmin - xmin) / std::min(dx, b.dx))) {
            return false;
        }

        // Check upper boundary compatibility
        if (!is_integral(std::abs(b.ymin - ymin) / std::min(dy, b.dy))) {
            return false;
        }

        return true;
    }

    Grid Grid::common_grid(const Grid &b) const {
        if (!compatible_with(b)) {
            throw std::runtime_error("Incompatible extents.");
        }

        double dx = std::min(this->dx, b.dx);
        double dy = std::min(this->dy, b.dy);

        double xmin = std::min(this->xmin, b.xmin);
        double ymax = std::max(this->ymax, b.ymax);

        double xmax = std::max(this->xmax, b.xmax);
        double ymin = std::min(this->ymin, b.ymin);

        int nx = (int) std::round((xmax - xmin) / dx);
        int ny = (int) std::round((ymax - ymin) / dy);

        xmax = std::max(xmax, xmin + nx*dx);
        ymin = std::min(ymin, ymax - ny*dy);

        return { xmin, ymin, xmax, ymax, dx, dy };
    }

    bool Grid::operator==(const Grid &b) const {
        return
            xmin == b.xmin &&
            ymin == b.ymin &&
            xmax == b.xmax &&
            ymax == b.ymax &&
            dx == b.dx &&
            dy == b.dy;
    }
}
