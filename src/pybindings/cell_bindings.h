#include <pybind11/pybind11.h>

#include "cell.h"

namespace py = pybind11;

namespace exactextract
{
    void bind_cell(py::module &m);
}