#include <pybind11/pybind11.h>
#include <pybind11/operators.h>

#include "box_bindings.h"
#include "box.h"

namespace py = pybind11;

namespace exactextract
{
    void bind_box(py::module &m)
    {
        py::class_<Box>(m, "Box")
            .def(py::init<double, double, double, double>())
            .def_readwrite("xmin", &Box::xmin)
            .def_readwrite("ymin", &Box::ymin)
            .def_readwrite("xmax", &Box::xmax)
            .def_readwrite("ymax", &Box::ymax)
            .def_static("maximum_finite", &Box::maximum_finite)
            .def_static("make_empty", &Box::make_empty)
            .def("width", &Box::width)
            .def("height", &Box::height)
            .def("area", &Box::area)
            .def("perimeter", &Box::perimeter)
            .def("intersects", &Box::intersects, py::arg("other"))
            .def("intersection", &Box::intersection, py::arg("other"))
            .def("translate", &Box::translate, py::arg("dx"), py::arg("dy"))
            .def("upper_left", &Box::upper_left)
            .def("upper_right", &Box::upper_right)
            .def("lower_left", &Box::lower_left)
            .def("lower_right", &Box::lower_right)
            .def("side", &Box::side, py::arg("c"))
            .def("crossing", &Box::crossing, py::arg("c1"), py::arg("c2"))
            .def("empty", &Box::empty)
            .def("expand_to_include", &Box::expand_to_include, py::arg("other"))
            .def(
                "contains", [](Box *self, const Box &b)
                { return self->contains(b); },
                py::arg("b"))
            .def(
                "contains", [](Box *self, const Coordinate &c)
                { return self->contains(c); },
                py::arg("c"))
            .def("strictly_contains", &Box::strictly_contains, py::arg("c"))
            .def(py::self == py::self);
    }
}