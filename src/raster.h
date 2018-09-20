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

#ifndef EXACTEXTRACT_RASTER_H
#define EXACTEXTRACT_RASTER_H

#include <limits>

#include "grid.h"
#include "matrix.h"

namespace exactextract {
    template<typename T>
    class AbstractRaster {
    public:
        explicit AbstractRaster(const Grid<bounded_extent> & ex) : m_grid{ex} {}

        size_t rows() const {
            return m_grid.rows();
        }

        size_t cols() const {
            return m_grid.cols();
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

            for (size_t i = 0; i < rows(); i++) {
                for (size_t j = 0; j < cols(); j++) {
                    if(operator()(i, j) != other(i, j)) {
                        if (!std::isnan(operator()(i, j)) || !std::isnan(other(i, j))) {
                            return false;
                        }
                    }
                }
            }

            return true;
        }
    private:
        Grid<bounded_extent> m_grid;
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

        Raster(const Box & box, size_t nrow, size_t ncol) :
                AbstractRaster<T>(Grid<bounded_extent>(box, std::round((box.xmax-box.xmin) / ncol), std::round((box.ymax-box.ymin) / nrow))),
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
        RasterView(const AbstractRaster<T> & r, Grid<bounded_extent> ex) : AbstractRaster<T>(ex), m_raster{r}, expanded{false} {
            double disaggregation_factor_x = r.xres() / ex.dx();
            double disaggregation_factor_y = r.yres() / ex.dy();

            if (disaggregation_factor_x != std::floor(disaggregation_factor_x) ||
                disaggregation_factor_y != std::floor(disaggregation_factor_y)) {
                throw std::runtime_error("Must construct view at resolution that is an integer multiple of original.");
            }

            if (disaggregation_factor_x < 0 || disaggregation_factor_y < 0) {
                throw std::runtime_error("Must construct view at equal or higher resolution than original.");
            }

            if (ex.xmin() < r.xmin() || ex.xmax() > r.xmax() || ex.ymin() < r.ymin() || ex.ymax() > r.ymax()) {
                expanded = true;
                //throw std::runtime_error("Constructed view must be smaller or same extent as original.");

            }

            m_x_off = static_cast<long>((ex.xmin() - r.xmin()) / ex.dx());
            m_y_off = static_cast<long>((r.ymax() - ex.ymax()) / ex.dy());
            m_rx = static_cast<size_t>(disaggregation_factor_x);
            m_ry = static_cast<size_t>(disaggregation_factor_y);
        }

        T operator()(size_t row, size_t col) const override {
            if (expanded) {
                if (row + m_y_off < 0) {
                    return std::numeric_limits<T>::quiet_NaN();
                }
                if (col + m_x_off < 0) {
                    return std::numeric_limits<T>::quiet_NaN();
                }
            }

            size_t i0 = (row + m_y_off) / m_ry;
            size_t j0 = (col + m_x_off) / m_rx;

            if (expanded) {
                if (i0 > m_raster.rows() - 1 || j0 > m_raster.cols() - 1) {
                    return std::numeric_limits<T>::quiet_NaN();
                }
            }

            return m_raster(i0, j0);
        }

    private:
        const AbstractRaster<T>& m_raster;
        bool expanded;

        long m_x_off;
        long m_y_off;
        size_t m_rx;
        size_t m_ry;
    };
}

#endif //EXACTEXTRACT_RASTER_H
