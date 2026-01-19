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
    template<typename T>
    static std::unique_ptr<NumPyRaster<T>>
    make_raster(const Grid<bounded_extent>& grid, const py::array& values, const py::object& nodata)
    {
        auto rast = std::make_unique<NumPyRaster<T>>(values, grid);

        if (!nodata.is_none()) {
            if constexpr (std::is_same_v<T, std::uint8_t>) {
                // Avoid exception when raster is read by GDAL
                // with type GDT_Byte (uint8) but having a negative
                // nodata value...
                int val = nodata.cast<int>();
                if (val >= 0) {
                    rast->set_nodata(static_cast<std::uint8_t>(val));
                }
            } else {
                rast->set_nodata(nodata.cast<T>());
            }
        }

        if (hasattr(values, "mask")) {
            py::object np = py::module_::import("numpy");
            py::function invert = np.attr("invert");
            py::array mask = values.attr("mask");
            py::array invmask = invert(mask);
            auto mask_rast = std::make_unique<NumPyRaster<std::int8_t>>(invmask, grid);
            rast->set_mask(std::move(mask_rast));
        }

        return rast;
    }

    RasterVariant read_box(const Box& box) override
    {
        auto cropped_grid = grid().crop(box);

        auto x0 = cropped_grid.col_offset(grid());
        auto y0 = cropped_grid.row_offset(grid());
        auto nx = cropped_grid.cols();
        auto ny = cropped_grid.rows();

        py::array rast_values = read_window(x0, y0, nx, ny);
        py::object nodata = nodata_value();

        if (py::isinstance<py::array_t<std::int8_t>>(rast_values)) {
            return make_raster<std::int8_t>(cropped_grid, rast_values, nodata);
        } else if (py::isinstance<py::array_t<std::uint8_t>>(rast_values)) {
            return make_raster<std::uint8_t>(cropped_grid, rast_values, nodata);
        } else if (py::isinstance<py::array_t<std::int16_t>>(rast_values)) {
            return make_raster<std::int16_t>(cropped_grid, rast_values, nodata);
        } else if (py::isinstance<py::array_t<std::uint16_t>>(rast_values)) {
            return make_raster<std::uint16_t>(cropped_grid, rast_values, nodata);
        } else if (py::isinstance<py::array_t<std::int32_t>>(rast_values)) {
            return make_raster<std::int32_t>(cropped_grid, rast_values, nodata);
        } else if (py::isinstance<py::array_t<std::uint32_t>>(rast_values)) {
            return make_raster<std::uint32_t>(cropped_grid, rast_values, nodata);
        } else if (py::isinstance<py::array_t<std::int64_t>>(rast_values)) {
            return make_raster<std::int64_t>(cropped_grid, rast_values, nodata);
        } else if (py::isinstance<py::array_t<std::uint64_t>>(rast_values)) {
            return make_raster<std::uint64_t>(cropped_grid, rast_values, nodata);
        } else if (py::isinstance<py::array_t<float>>(rast_values)) {
            return make_raster<float>(cropped_grid, rast_values, nodata);
        } else {
            return make_raster<double>(cropped_grid, rast_values, nodata);
        }
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

    virtual py::object srs_wkt() const
    {
        return py::none();
    }

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

    py::object nodata_value() const override
    {
        PYBIND11_OVERRIDE_PURE(py::object, PyRasterSource, nodata_value);
    }

    py::object srs_wkt() const override
    {
        PYBIND11_OVERRIDE(py::str, PyRasterSourceBase, srs_wkt);
    }
};

void
bind_raster_source(py::module& m)
{
    py::class_<RasterSource>(m, "RasterSourceBase")
      .def("set_name", &RasterSource::set_name, py::arg("name"))
      .def("name", &RasterSource::name)
      .def("thread_safe", &RasterSource::thread_safe);

    py::class_<PyRasterSourceBase>(m, "PyRasterSourceBase");

    py::class_<PyRasterSource, PyRasterSourceBase, RasterSource>(m, "RasterSource")
      .def(py::init<>())
      .def("extent", &PyRasterSource::extent)
      .def("res", &PyRasterSource::res)
      .def("srs_wkt", &PyRasterSourceBase::srs_wkt)
      .def("nodata_value", &PyRasterSource::nodata_value)
      .def("read_window", &PyRasterSource::read_window)
      .def("thread_safe", &PyRasterSourceBase::thread_safe);
}
}
