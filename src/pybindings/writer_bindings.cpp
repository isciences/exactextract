#include <map>
#include <list>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "processor.h"
#include "gdal_dataset_wrapper.h"
#include "utils.h"
#include "output_writer.h"
#include "gdal_writer.h"
#include "map_writer.h"
#include "writer_bindings.h"

namespace py = pybind11;

namespace exactextract
{
    void bind_writer(py::module &m)
    {
        py::class_<OutputWriter>(m, "OutputWriter")
            .def("write", &OutputWriter::write, py::arg("fid"))
            .def("add_operation", &OutputWriter::add_operation, py::arg("op"))
            .def("set_registry", &OutputWriter::set_registry, py::arg("reg"))
            .def("finish", &OutputWriter::finish);

        py::class_<GDALWriter, OutputWriter>(m, "GDALWriter")
            .def(py::init<const std::string &>(), py::arg("filename"))
            .def_static("get_driver_name", &GDALWriter::get_driver_name, py::arg("filename"))
            .def("add_id_field", &GDALWriter::add_id_field, py::arg("field_name"), py::arg("field_type"))
            .def("copy_id_field", &GDALWriter::copy_id_field, py::arg("w"));

        py::class_<MapWriter, OutputWriter>(m, "MapWriter")
            .def(py::init<>())
            .def("reset_operation", &MapWriter::reset_operation)
            .def("clear", &MapWriter::clear)
            .def("get_map", &MapWriter::get_map);
    }
}