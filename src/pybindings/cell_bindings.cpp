#include <pybind11/pybind11.h>

#include "cell_bindings.h"
#include "cell.h"

namespace py = pybind11;

namespace exactextract
{
    void bind_cell(py::module &m)
    {
        py::class_<Cell>(m, "Cell")
            .def(py::init<double, double, double, double>())
            .def(py::init<Box>())
            .def("force_exit", &Cell::force_exit)
            .def("width", &Cell::width)
            .def("height", &Cell::height)
            .def("area", &Cell::area)
            .def("covered_fraction", &Cell::covered_fraction)
            .def("last_traversal", &Cell::last_traversal)
            .def("box", &Cell::box)
            .def("take", &Cell::take, py::arg("c"), py::arg("prev_original") = nullptr);
    }
}