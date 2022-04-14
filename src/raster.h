// Copyright (c) 2018-2021 ISciences, LLC.
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

#ifndef EXACTEXTRACT_RASTER_H
#define EXACTEXTRACT_RASTER_H

#include <array>
#include <cmath>
#include <limits>

#include "grid.h"
#include "matrix.h"

namespace exactextract {
    template<typename T>
    class AbstractRaster {
    public:
        using value_type = T;

        explicit AbstractRaster(const Grid<bounded_extent> & ex) :
            m_grid{ex},
            m_nodata{std::is_floating_point<T>::value ? std::numeric_limits<T>::quiet_NaN() : std::numeric_limits<T>::min()},
            m_has_nodata{false}
            {}

        AbstractRaster(const Grid<bounded_extent> & ex, const T& nodata_val) : m_grid{ex}, m_nodata{nodata_val}, m_has_nodata{true}  {}

        virtual ~AbstractRaster() = default;

        size_t rows() const {
            return m_grid.rows();
        }

        size_t cols() const {
            return m_grid.cols();
        }

        size_t size() const {
            return rows() * cols();
        }

        double xres() const {
            return m_grid.dx();
        }

        double yres() const {
            return m_grid.dy();
        }

        double xmin() const {
            return m_grid.xmin();
        }

        double ymin() const {
            return m_grid.ymin();
        }

        double xmax() const {
            return m_grid.xmax();
        }

        double ymax() const {
            return m_grid.ymax();
        }

        const Grid<bounded_extent>& grid() const {
            return m_grid;
        }

        virtual T operator()(size_t row, size_t col) const = 0;

        bool has_nodata() const { return m_has_nodata; }

        T nodata() const { return m_nodata; }

        bool get(size_t row, size_t col, T & val) const {
            val = operator()(row, col);

            if (m_has_nodata && val == m_nodata) {
                return false;
            }
            if (std::is_floating_point<T>::value && std::isnan(val)) {
                return false;
            }

            return true;
        }

        void set_nodata(const T & val) {
            m_has_nodata = true;
            m_nodata = val;
        }

        bool operator==(const AbstractRaster<T> & other) const {
            if (rows() != other.rows())
                return false;

            if (cols() != other.cols())
                return false;

            if (xres() != other.xres())
                return false;

            if (yres() != other.yres())
                return false;

            if (xmin() != other.xmin())
                return false;

            if (ymin() != other.ymin())
                return false;

            // Do the rasters differ in their definition of NODATA? If so, we need to do some more detailed
            // checking below.
            bool nodata_differs = has_nodata() != other.has_nodata() ||
                    (has_nodata() && other.has_nodata() && nodata() != other.nodata());

            for (size_t i = 0; i < rows(); i++) {
                for (size_t j = 0; j < cols(); j++) {
                    if(operator()(i, j) != other(i, j)) {
                        // Override default behavior of NAN != NAN
                        if (!std::isnan(operator()(i, j)) || !std::isnan(other(i, j))) {
                            return false;
                        }
                    } else if (nodata_differs && (operator()(i, j) == nodata() || other(i, j) == other.nodata())) {
                        // For data types that do not have NAN, or even for floating point types where the user has
                        // selected some other value to represent NODATA, we need to reverse a positive equality test
                        // where the value is considered to be NODATA in one raster but not the other.
                        return false;
                    }
                }
            }

            return true;
        }

        bool operator!=(const AbstractRaster<T> & other) const {
            return !(operator==(other));
        }

        class Iterator : public std::iterator<std::forward_iterator_tag, T> {
        public:
            Iterator(const AbstractRaster<T>* r, size_t i, size_t j) :
                    m_rast(r), m_row(i), m_col(j) {}

            const T& operator*() const {
                m_val = m_rast->operator()(m_row, m_col);
                return m_val;
            }

            Iterator& operator++() {
                m_col++;
                if (m_col == m_rast->cols()) {
                    m_col = 0;
                    m_row++;
                }
                return *this;
            }

            friend bool operator==(const Iterator& a,
                                   const Iterator& b) {
                return a.m_rast == b.m_rast && a.m_row == b.m_row && a.m_col == b.m_col;
            }

            friend bool operator!=(const Iterator& a,
                                   const Iterator& b) {
                return a.m_row != b.m_row || a.m_col != b.m_col || a.m_rast != b.m_rast;
            }

        private:
            const AbstractRaster<T>* m_rast;
            mutable T m_val;
            size_t m_row;
            size_t m_col;
        };

        Iterator begin() const {
            return Iterator(this, 0, 0);
        }

        Iterator end() const {
            return Iterator(this, rows(), 0);
        }
    private:
        Grid<bounded_extent> m_grid;
        T m_nodata;
        bool m_has_nodata;
    };

    template<typename T>
    class Raster : public AbstractRaster<T> {
    public:
        Raster(Matrix<T>&& values, const Box & box) : AbstractRaster<T>(
                Grid<bounded_extent>(
                        box,
                        (box.xmax - box.xmin) / values.cols(),
                        (box.ymax - box.ymin) / values.rows())),
                m_values{std::move(values)} {}

        Raster(Matrix<T>&& values, const Grid<bounded_extent> & g) :
            AbstractRaster<T>(g),
            m_values{std::move(values)} {}

        Raster(const Box & box, size_t nrow, size_t ncol) :
                AbstractRaster<T>(Grid<bounded_extent>(box, (box.xmax-box.xmin) / ncol, (box.ymax-box.ymin) / nrow)),
                m_values{nrow, ncol}
                {}

        explicit Raster(const Grid<bounded_extent> & ex) :
            AbstractRaster<T>(ex),
            m_values{ex.rows(), ex.cols()}
            {}

        Matrix<T>& data() {
            return m_values;
        }

        T& operator()(size_t row, size_t col) {
            return m_values(row, col);
        }

        T operator()(size_t row, size_t col) const override {
            return m_values(row, col);
        }

    private:
        Matrix<T> m_values;
    };

    template<typename T>
    class RasterView : public AbstractRaster<T>{
    public:
        // Construct a view of a raster r at an extent ex that is larger
        // and/or of finer resolution than r
        RasterView(const AbstractRaster<T> & r, Grid<bounded_extent> ex) :
            AbstractRaster<T>(ex),
                    m_raster{r},
                    m_x_off{0},
                    m_y_off{0},
                    m_rx{1},
                    m_ry{1} {
            if (!this->grid().empty()) {
                double disaggregation_factor_x = r.xres() / ex.dx();
                double disaggregation_factor_y = r.yres() / ex.dy();

                if (std::abs(disaggregation_factor_x - std::round(disaggregation_factor_x)) > 1e-6 ||
                    std::abs(disaggregation_factor_y - std::round(disaggregation_factor_y)) > 1e-6) {
                    throw std::runtime_error("Must construct view at resolution that is an integer multiple of original.");
                }

                if (disaggregation_factor_x < 0 || disaggregation_factor_y < 0) {
                    throw std::runtime_error("Must construct view at equal or higher resolution than original.");
                }

                m_x_off = static_cast<long>(std::round((ex.xmin() - r.xmin()) / ex.dx()));
                m_y_off = static_cast<long>(std::round((r.ymax() - ex.ymax()) / ex.dy()));
                m_rx = static_cast<size_t>(std::round(disaggregation_factor_x));
                m_ry = static_cast<size_t>(std::round(disaggregation_factor_y));
            }

            if (r.has_nodata()) {
                this->set_nodata(r.nodata());
            }
        }

        T operator()(size_t row, size_t col) const override {
            if (m_raster.grid().empty()) {
                return this->nodata();
            }

            if (m_x_off < 0 && static_cast<size_t>(-m_x_off) > col) {
                return this->nodata();
            }
            if (m_y_off < 0 && static_cast<size_t>(-m_y_off) > row) {
                return this->nodata();
            }

            size_t i0 = (row + m_y_off) / m_ry;
            size_t j0 = (col + m_x_off) / m_rx;

            if (i0 > m_raster.rows() - 1 || j0 > m_raster.cols() - 1) {
                return this->nodata();
            }

            return m_raster(i0, j0);
        }

    private:
        const AbstractRaster<T>& m_raster;

        long m_x_off;
        long m_y_off;
        size_t m_rx;
        size_t m_ry;
    };


    template<typename T>
    std::ostream& operator<<(std::ostream & os, const AbstractRaster<T> & m) {
        for (size_t i = 0; i < m.rows(); i++) {
            for (size_t j = 0; j < m.cols(); j++) {
                if (m(i, j) != 0) {
                    os << std::right << std::fixed << std::setw(10) << std::setprecision(6) <<
                       m(i, j) << " ";
                } else {
                    os << "           ";
                }
            }
            os << std::endl;
        }

        return os;
    }
}

#endif //EXACTEXTRACT_RASTER_H
