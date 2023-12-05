// Copyright (c) 2023 ISciences, LLC.
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

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <utility>

#include "raster_source.h"
#include "raster_source_bindings.h"

namespace py = pybind11;

namespace exactextract {
template<typename T>
class NumPyRaster : public AbstractRaster<T>
{
  public:
    using Unchecked2DArrayProxy = decltype(std::declval<py::array_t<T>>().template unchecked<2>());

    NumPyRaster(py::array_t<T, py::array::c_style | py::array::forcecast> array,
                const Grid<bounded_extent>& g)
      : AbstractRaster<T>(g)
      , m_array(array)
      , m_array_proxy(array.template unchecked<2>())
    {
    }

    T operator()(std::size_t row, std::size_t col) const override
    {
        return m_array_proxy(row, col);
    }

  private:
    py::array_t<T> m_array;
    Unchecked2DArrayProxy m_array_proxy;
};

class PyRasterSourceBase : public RasterSource
{

  public:
    std::unique_ptr<AbstractRaster<double>> read_box(const Box& box) override
    {
        auto cropped_grid = grid().crop(box);

        if (!cropped_grid.empty()) {
            auto x0 = cropped_grid.col_offset(grid());
            auto y0 = cropped_grid.row_offset(grid());
            auto nx = cropped_grid.cols();
            auto ny = cropped_grid.rows();

            py::array rast_values = read_window(x0, y0, nx, ny);
            py::object nodata = nodata_value();

            auto rast = std::make_unique<NumPyRaster<double>>(rast_values, cropped_grid);

            if (!nodata.is_none()) {
                rast->set_nodata(nodata.cast<double>());
            }

            return rast;
        }

        return nullptr;
    }

    const Grid<bounded_extent>& grid() const override
    {
        if (m_grid == nullptr) {
            py::sequence grid_ext = extent();

            if (grid_ext.size() != 4) {
                throw std::runtime_error("Expected 4 elements in extent");
            }

            py::sequence grid_res = res();

            if (grid_res.size() != 2) {
                throw std::runtime_error("Expected 2 elements in resolution");
            }

            Box b{ grid_ext[0].cast<double>(), grid_ext[1].cast<double>(), grid_ext[2].cast<double>(), grid_ext[3].cast<double>() };

            double dx = grid_res[0].cast<double>();
            double dy = grid_res[1].cast<double>();

            m_grid = std::make_unique<Grid<bounded_extent>>(b, dx, dy);
        }

        return *m_grid;
    }

    virtual py::array read_window(int x0, int y0, int nx, int ny) const = 0;

    virtual py::object extent() const = 0;

    virtual py::object res() const = 0;

    virtual py::object nodata_value() const = 0;

  private:
    mutable std::unique_ptr<Grid<bounded_extent>> m_grid;
};

class PyRasterSource : public PyRasterSourceBase
{

  public:
    py::object extent() const override
    {
        PYBIND11_OVERRIDE_PURE(py::tuple, PyRasterSource, extent);
    }

    py::object res() const override
    {
        PYBIND11_OVERRIDE_PURE(py::object, PyRasterSource, res);
    }

    py::array read_window(int x0, int y0, int nx, int ny) const override
    {
        PYBIND11_OVERRIDE_PURE(py::array, PyRasterSource, read_window, x0, y0, nx, ny);
    }

    py::object nodata_value() const override {
        PYBIND11_OVERRIDE_PURE(py::object, PyRasterSource, nodata_value);
    }
};

void
bind_raster_source(py::module& m)
{
    py::class_<RasterSource>(m, "RasterSourceBase")
      .def("set_name", &RasterSource::set_name, py::arg("name"))
      .def("name", &RasterSource::name);

    py::class_<PyRasterSourceBase>(m, "PyRasterSourceBase");

    py::class_<PyRasterSource, PyRasterSourceBase, RasterSource>(m, "RasterSource")
      .def(py::init<>())
      .def("extent", &PyRasterSource::extent)
      .def("res", &PyRasterSource::res)
      .def("nodata_value", &PyRasterSource::nodata_value)
      .def("read_window", &PyRasterSource::read_window);
}
}
