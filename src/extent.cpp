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

#include "extent.h"

#include <cmath>
#include <memory>
#include <stdexcept>

namespace exactextract {

    Extent::Extent(double xmin, double ymin, double xmax, double ymax, double dx, double dy) :
            xmin{xmin},
            ymin{ymin},
            xmax{xmax},
            ymax{ymax},
            dx{dx},
            dy{dy},
            m_first_row{0},
            m_first_col{0} {
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

    size_t Extent::get_column(double x) const {
        if (x < xmin || x > xmax) {
            throw std::out_of_range("x");
        }

        // special case for xmax, returning the cell for which xmax is the
        // right-hand side
        if (x == xmax) {
            return m_num_cols - 1;
        }

        return (size_t) std::floor((x - xmin) / dx);
    }

    size_t Extent::get_row(double y) const {
        if (y < ymin || y > ymax)
            throw std::out_of_range("y");

        // special case for ymin, returning the cell for which ymin is the
        // bottom
        if (y == ymin)
            return m_num_rows - 1;

        return (size_t) std::floor((ymax - y) / dy);
    }

    Extent Extent::shrink_to_fit(const Box &b) const {
        return shrink_to_fit(b.xmin, b.ymin, b.xmax, b.ymax);
    }

    Extent Extent::shrink_to_fit(double x0, double y0, double x1, double y1) const {
        if (x0 < xmin || y0 < ymin || x1 > xmax || y1 > ymax) {
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
        Extent reduced(
                snapped_xmin,
                snapped_ymax - numRows * dy,
                snapped_xmin + numCols * dx,
                snapped_ymax,
                dx,
                dy);

        if (x0 < reduced.xmin || y0 < reduced.ymin || x1 > reduced.xmax || y1 > reduced.ymax) {
            throw std::runtime_error("Shrink operation failed.");
        }

        reduced.m_first_col = col0;
        reduced.m_first_row = row1;

        return reduced;
    }

    std::unique_ptr<Cell> Extent::get_cell_ptr(size_t row, size_t col) const {
        return std::make_unique<Cell>(
                xmin + col * dx,
                ymax - (row + 1) * dy,
                xmin + (col + 1) * dx,
                ymax - row * dy
        );
    }

}