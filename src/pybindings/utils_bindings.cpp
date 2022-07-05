#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "utils_bindings.h"
#include "utils.h"

namespace py = pybind11;

namespace exactextract
{
    void bind_utils(py::module &m)
    {
        py::class_<StatDescriptor>(m, "StatDescriptor")
            .def_readwrite("name", &StatDescriptor::name)
            .def_readwrite("values", &StatDescriptor::values)
            .def_readwrite("weights", &StatDescriptor::weights)
            .def_readwrite("stat", &StatDescriptor::stat)
            .def_static("from_descriptor", &parse_stat_descriptor, py::arg("descriptor"));

        m.def("parse_dataset_descriptor", &parse_dataset_descriptor);
        m.def("parse_raster_descriptor", &parse_raster_descriptor);
    }
}