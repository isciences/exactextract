#include <pybind11/pybind11.h>
#include <pybind11/operators.h>

#include "coordinate_bindings.h"
#include "coordinate.h"

namespace py = pybind11;

namespace exactextract
{
    void bind_coordinate(py::module &m)
    {
        py::class_<Coordinate>(m, "Coordinate")
            .def(py::init<>())
            .def(py::init<double, double>())
            .def_readwrite("x", &Coordinate::x)
            .def_readwrite("y", &Coordinate::y)
            .def("equals", &Coordinate::equals, py::arg("other"), py::arg("tol"))
            .def(py::self == py::self)
            .def(py::self != py::self);
    }
}