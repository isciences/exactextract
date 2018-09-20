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

namespace exactextract {
    Box grid_cell(const Grid<bounded_extent> & grid, size_t row, size_t col) {
        // The ternary clauses below are used to make sure that the cells along
        // the right and bottom edges of our grid are slightly larger than m_dx,dy
        // if needed to make sure that we capture our whole extent. This is necessary
        // because xmin + nx*m_dx may be less than xmax because of floating point
        // errors.
        return {
                grid.xmin() + col * grid.dx(),
                row == (grid.rows() - 1) ? grid.ymin() : (grid.ymax() - (row + 1) * grid.dy()),
                col == (grid.cols() - 1) ? grid.xmax() : (grid.xmin() + (col + 1) * grid.dx()),
                grid.ymax() - row * grid.dy()

        };
    }

    Box grid_cell(const Grid<infinite_extent> & grid, size_t row, size_t col) {
        double xmin, xmax, ymin, ymax;

        if (col == 0) {
            xmin = std::numeric_limits<double>::lowest();
        } else {
            xmin = grid.xmin() + (col - 1) * grid.dx();
        }

        switch(grid.cols() - col) {
            case 1: xmax = std::numeric_limits<double>::max(); break;
            case 2: xmax = grid.xmax(); break;
            default: xmax = grid.xmin() + col*grid.dx();
        }

        if (row == 0) {
            ymax = std::numeric_limits<double>::max();
        } else {
            ymax = grid.ymax() - (row - 1) * grid.dy();
        }

        switch(grid.rows() - row) {
            case 1: ymin = std::numeric_limits<double>::lowest(); break;
            case 2: ymin = grid.ymin(); break;
            default: ymin = grid.ymax() - row*grid.dy();
        }

        return { xmin, ymin, xmax, ymax };
    }
}
