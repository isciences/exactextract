// Copyright (c) 2023-2024 ISciences, LLC.
// All rights reserved.
//
// This software is licensed under the Apache License, Version 2.0 (the "License").
// You may not use this file except in compliance with the License. You may
// obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h> // needed to bind Operation::field_names

#include "operation.h"
#include "operation_bindings.h"

#include "map_feature.h"
#include "raster_source.h"
#include "utils.h"

namespace py = pybind11;

namespace exactextract {

class PythonOperation : public Operation
{
  public:
    PythonOperation(py::function fun,
                    std::string p_name,
                    RasterSource* p_values,
                    RasterSource* p_weights,
                    bool call_with_weights,
                    Operation::ArgMap args = {})
      : Operation("", p_name, p_values, p_weights, args)
      , m_function(fun)
      , m_call_with_weights(call_with_weights)
    {
    }

    bool requires_stored_coverage_fractions() const override
    {
        return true;
    }

    bool requires_stored_values() const override
    {
        return true;
    }

    bool requires_stored_weights() const override
    {
        return m_call_with_weights;
    }

    std::unique_ptr<Operation> clone() const override
    {
        return std::make_unique<PythonOperation>(*this);
    }

    Feature::ValueType result_type() const override
    {
        MapFeature mf;
        set_empty_result(mf);
        return mf.field_type(name);
    }

    virtual void set_result(const StatsRegistry::RasterStatsVariant& stats_variant, Feature& f_out) const override
    {
        py::object result = std::visit([this](const auto& stats) -> py::object {
            // TODO avoid array copies here?
            // Review solutions at https://github.com/pybind/pybind11/issues/1042
            if (m_call_with_weights) {
                return m_function(
                  py::array(py::cast(stats.values())),
                  py::array(py::cast(stats.coverage_fractions())),
                  py::array(py::cast(stats.weights())));
            }

            return m_function(
              py::array(py::cast(stats.values())),
              py::array(py::cast(stats.coverage_fractions())));
        },
                                       stats_variant);

        // TODO detect if f_out can handle a py::object directly?
        if (py::isinstance<py::int_>(result)) {
            f_out.set(name, result.cast<std::int64_t>());
        } else if (py::isinstance<py::float_>(result)) {
            f_out.set(name, result.cast<double>());
        } else if (py::isinstance<py::str>(result)) {
            f_out.set(name, result.cast<std::string>());
        } else if (py::isinstance<py::array_t<std::int8_t>>(result)) {
            f_out.set(name, result.cast<std::vector<std::int8_t>>());
        } else if (py::isinstance<py::array_t<std::int16_t>>(result)) {
            f_out.set(name, result.cast<std::vector<std::int16_t>>());
        } else if (py::isinstance<py::array_t<std::int32_t>>(result)) {
            f_out.set(name, result.cast<std::vector<std::int32_t>>());
        } else if (py::isinstance<py::array_t<std::int64_t>>(result)) {
            f_out.set(name, result.cast<std::vector<std::int64_t>>());
        } else if (py::isinstance<py::array_t<float>>(result)) {
            f_out.set(name, result.cast<std::vector<float>>());
        } else if (py::isinstance<py::array_t<double>>(result)) {
            f_out.set(name, result.cast<std::vector<double>>());
        } else if (py::isinstance<py::none>(result)) {
            // do nothing
        } else {
            throw std::runtime_error("Unhandled type returned from Python operation");
        }
    }

  private:
    py::function m_function;
    const bool m_call_with_weights;
};

void
bind_operation(py::module& m)
{
    py::class_<Operation>(m, "Operation")
      .def(py::init(&Operation::create))
      .def("grid", &Operation::grid)
      .def("weighted", &Operation::weighted)
      .def_readonly("stat", &Operation::stat)
      .def_readonly("name", &Operation::name)
      .def_readonly("values", &Operation::values)
      .def_readonly("weights", &Operation::weights);

    py::class_<PythonOperation, Operation>(m, "PythonOperation")
      .def(py::init<py::function, std::string, RasterSource*, RasterSource*, bool>());

    m.def("prepare_operations", py::overload_cast<const std::vector<std::string>&, const std::vector<RasterSource*>&, const std::vector<RasterSource*>&>(&prepare_operations));
}
}
