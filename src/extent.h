#ifndef EXTENT_H
#define EXTENT_H

#include <memory>

#include "box.h"
#include "cell.h"

class Extent {
    public:

    double xmin;
    double ymin;
    double xmax;
    double ymax;

    double dx;
    double dy;

    Extent(double xmin, double ymin, double xmax, double ymax, double dx, double dy);
    Extent() : xmin{0}, ymin{0}, xmax{0}, ymax{0}, dx{0}, dy{0} {}

    size_t get_column(double x) const;
    size_t get_row(double x) const;
    size_t rows() const { return m_num_rows; }
    size_t cols() const { return m_num_cols; }
    size_t row_offset() const { return m_first_row; }
    size_t col_offset() const { return m_first_col; }

    double x_for_col(size_t col) const { return xmin + (col + 0.5) * dx; }

    double y_for_row(size_t row) const { return ymax - (row + 0.5) * dy; }

    std::unique_ptr<Cell> get_cell_ptr(size_t row, size_t col) const;
    Extent shrink_to_fit(double x0, double y0, double x1, double y1) const;
    Extent shrink_to_fit(const Box & b) const;

    private:
    size_t m_first_row;
    size_t m_first_col;

    size_t m_num_rows;
    size_t m_num_cols;
};

#endif