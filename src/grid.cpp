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

    Grid<infinite_extent> make_infinite(const Grid<bounded_extent> & grid) {
        return { grid.extent(), grid.dx(), grid.dy() };
    }

    Grid<bounded_extent> make_finite(const Grid<infinite_extent> & grid) {
        return { grid.extent(), grid.dx(), grid.dy() };
    }

    std::vector<Grid<bounded_extent>> subdivide(const Grid<bounded_extent> & grid, size_t max_size) {
        if (grid.size() < max_size) {
            return { grid };
        }

        size_t cols_per_block = std::min(max_size, grid.cols());
        size_t rows_per_block = max_size / cols_per_block;

        size_t col_blocks = (grid.cols() - 1) / cols_per_block + 1;
        size_t row_blocks = (grid.rows() - 1) / rows_per_block + 1;

        std::vector<Grid<bounded_extent>> subgrids;
        for (size_t i = 0; i < row_blocks; i++) {
            for (size_t j = 0; j < col_blocks; j++) {
                double xmin = grid.xmin() + grid.dx()*cols_per_block*j;
                double xmax = j == (col_blocks - 1) ? grid.xmax() : (grid.xmin() + grid.dx()*cols_per_block*(j+1));
                double ymax = grid.ymax() - grid.dy()*rows_per_block*i;
                double ymin = i == (row_blocks - 1) ? grid.ymin() : (grid.ymax() - grid.dy()*rows_per_block*(i+1));

                Box reduced = {xmin, ymin, xmax, ymax};
                subgrids.emplace_back(reduced, grid.dx(), grid.dy());
            }
        }

        return subgrids;
    }
}
