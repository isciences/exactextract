#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace exactextract
{
    void bind_gdal_dataset_wrapper(py::module &m);
}