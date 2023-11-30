// Copyright (c) 2023 ISciences, LLC.
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
      .def("weighted", &Operation::weighted)
      .def("grid", &Operation::grid)
      .def_readonly("stat", &Operation::stat)
      .def_readonly("name", &Operation::name)
      .def_readonly("values", &Operation::values)
      .def_readonly("weights", &Operation::weights);
}
}
