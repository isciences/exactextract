#include <pybind11/pybind11.h>

#include "operation_bindings.h"
#include "raster_source.h"
#include "operation.h"

namespace py = pybind11;

namespace exactextract
{
    void bind_operation(py::module &m)
    {
        py::class_<Operation>(m, "Operation")
            .def(py::init<std::string, std::string, RasterSource *, RasterSource *>())
            .def("weighted", &Operation::weighted)
            .def("grid", &Operation::grid)
            .def_readonly("stat", &Operation::stat)
            .def_readonly("name", &Operation::name)
            .def_readonly("values", &Operation::values)
            .def_readonly("weights", &Operation::weights);
    }
}