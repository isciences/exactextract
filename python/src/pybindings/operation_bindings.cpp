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

#include <pybind11/pybind11.h>
#include <pybind11/stl.h> // needed to bind Operation::field_names

#include "operation.h"
#include "operation_bindings.h"
#include "raster_source.h"

namespace py = pybind11;

namespace exactextract {
void
bind_operation(py::module& m)
{
    py::class_<Operation>(m, "Operation")
      .def(py::init<std::string, std::string, RasterSource*, RasterSource*>())
      .def_property_readonly("column_names_known", &Operation::column_names_known)
      .def("field_names", &Operation::field_names)
      .def("grid", &Operation::grid)
      .def("weighted", &Operation::weighted)
      .def_readonly("stat", &Operation::stat)
      .def_readonly("name", &Operation::name)
      .def_readonly("values", &Operation::values)
      .def_readonly("weights", &Operation::weights);
}
}
