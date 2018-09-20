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

#ifndef EXACTEXTRACT_LEGACY_GRID_H
#define EXACTEXTRACT_LEGACY_GRID_H

#include <memory>

#include "box.h"
#include "cell.h"

namespace exactextract {

    class LegacyGrid {
    public:
        double xmin;
        double ymin;
        double xmax;
        double ymax;

        double dx;
        double dy;

        LegacyGrid(double xmin, double ymin, double xmax, double ymax, double dx, double dy);

        LegacyGrid() : xmin{0}, ymin{0}, xmax{0}, ymax{0}, dx{0}, dy{0}, m_num_rows{0}, m_num_cols{0} {}

        size_t get_column(double x) const;

        size_t get_row(double x) const;

        size_t rows() const { return m_num_rows; }

        size_t cols() const { return m_num_cols; }

        size_t row_offset(const LegacyGrid & other) const { return static_cast<size_t>(std::round((other.ymax - ymax) / dy)); }

        size_t col_offset(const LegacyGrid & other) const { return static_cast<size_t>(std::round((xmin - other.xmin) / dx)); }

        double x_for_col(size_t col) const { return xmin + (col + 0.5) * dx; }

        double y_for_row(size_t row) const { return ymax - (row + 0.5) * dy; }

        std::unique_ptr<Cell> get_cell_ptr(size_t row, size_t col) const;

        LegacyGrid shrink_to_fit(double x0, double y0, double x1, double y1) const;

        LegacyGrid shrink_to_fit(const Box &b) const;

        bool compatible_with(const LegacyGrid & b) const;

        LegacyGrid common_grid(const LegacyGrid &b) const;

        bool operator==(const LegacyGrid & b) const;

    private:
        size_t m_num_rows;
        size_t m_num_cols;
    };

}

#endif