#include <map>
#include <list>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "processor.h"
#include "gdal_dataset_wrapper.h"
#include "utils.h"
#include "output_writer.h"
#include "gdal_writer.h"
#include "writer_bindings.h"

namespace py = pybind11;

namespace exactextract
{
    MapWriter::MapWriter() {}

    MapWriter::~MapWriter() {}

    void MapWriter::add_operation(const Operation &op)
    {
        m_ops.push_back(&op);
    }

    void MapWriter::reset_operation()
    {
        m_ops.clear();
    }

    void MapWriter::set_registry(const StatsRegistry *reg)
    {
        m_reg = reg;
    }

    void MapWriter::write(const std::string &fid)
    {
        if (!m_dict.count(fid))
        {
            m_dict[fid] = std::list<double>();
        }
        for (const auto &op : m_ops)
        {
            if (m_reg->contains(fid, *op))
            {
                const auto &stats = m_reg->stats(fid, *op);
                auto fetcher = op->result_fetcher();

                auto val = fetcher(stats);
                if (val.has_value())
                {
                    m_dict[fid].push_back(val.value());
                }
                else
                {
                    m_dict[fid].push_back(std::numeric_limits<double>::quiet_NaN());
                }
            }
        }
    }

    void MapWriter::clear()
    {
        m_dict.clear();
    }

    std::map<const std::string, std::list<double>> MapWriter::get_map()
    {
        return m_dict;
    }

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