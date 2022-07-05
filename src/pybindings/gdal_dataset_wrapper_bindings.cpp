#include <pybind11/pybind11.h>

#include "gdal_dataset_wrapper_bindings.h"
#include "gdal_dataset_wrapper.h"

namespace py = pybind11;

namespace exactextract
{
    void bind_gdal_dataset_wrapper(py::module &m)
    {
        py::class_<GDALDatasetWrapper>(m, "GDALDatasetWrapper")
            .def(py::init<const std::string &, const std::string &, const std::string &>())
            .def("feature_field", &GDALDatasetWrapper::feature_field)
            .def("id_field", &GDALDatasetWrapper::id_field);
    }
}