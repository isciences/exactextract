#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace exactextract
{
    template <typename extent_tag>
    void bind_grid_template(py::module m, const char *class_name);

    void bind_grid(py::module &m);
}