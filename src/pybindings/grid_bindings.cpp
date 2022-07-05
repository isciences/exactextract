#include <pybind11/pybind11.h>
#include <pybind11/operators.h>

#include "grid_bindings.h"
#include "grid.h"

namespace py = pybind11;

namespace exactextract
{
    template <typename extent_tag>
    void bind_grid_template(py::module m, const char *class_name)
    {
        py::class_<Grid<extent_tag>>(m, class_name)
            .def(py::init<Box, double, double>())
            .def_static("make_empty", &Grid<extent_tag>::make_empty)
            .def("get_column", &Grid<extent_tag>::get_column, py::arg("x"))
            .def("get_row", &Grid<extent_tag>::get_row, py::arg("y"))
            .def("empty", &Grid<extent_tag>::empty)
            .def("rows", &Grid<extent_tag>::rows)
            .def("cols", &Grid<extent_tag>::cols)
            .def("size", &Grid<extent_tag>::size)
            .def("xmin", &Grid<extent_tag>::xmin)
            .def("xmax", &Grid<extent_tag>::xmax)
            .def("ymin", &Grid<extent_tag>::ymin)
            .def("ymax", &Grid<extent_tag>::ymax)
            .def("dx", &Grid<extent_tag>::dx)
            .def("dy", &Grid<extent_tag>::dy)
            .def("extent", &Grid<extent_tag>::extent)
            .def("row_offset", &Grid<extent_tag>::row_offset, py::arg("other"))
            .def("col_offset", &Grid<extent_tag>::col_offset, py::arg("other"))
            .def("x_for_col", &Grid<extent_tag>::x_for_col, py::arg("col"))
            .def("y_for_row", &Grid<extent_tag>::y_for_row, py::arg("row"))
            .def("crop", &Grid<extent_tag>::crop, py::arg("b"))
            .def("shrink_to_fit", &Grid<extent_tag>::shrink_to_fit, py::arg("b"))
            .def(py::self == py::self)
            .def(py::self != py::self);
    }
    void bind_grid(py::module &m)
    {
        bind_grid_template<infinite_extent>(m, "Grid__infinite_extent");
        bind_grid_template<bounded_extent>(m, "Grid__bounded_extent");
    }

}