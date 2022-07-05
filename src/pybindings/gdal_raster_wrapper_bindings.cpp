#include <pybind11/pybind11.h>

#include "gdal_raster_wrapper_bindings.h"
#include "raster_source.h"
#include "gdal_raster_wrapper.h"

namespace py = pybind11;

namespace exactextract
{
    void bind_gdal_raster_wrapper(py::module &m)
    {
        py::class_<RasterSource>(m, "RasterSource")
            .def("set_name", &RasterSource::set_name, py::arg("name"))
            .def("name", &RasterSource::name);

        py::class_<GDALRasterWrapper, RasterSource>(m, "GDALRasterWrapper")
            .def(py::init<const std::string &, int>())
            .def("grid", &GDALRasterWrapper::grid)
            .def("read_box", &GDALRasterWrapper::read_box, py::arg("box"));
    }
}