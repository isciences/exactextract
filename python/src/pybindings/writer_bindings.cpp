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

#include "output_writer.h"
#include "writer_bindings.h"

namespace py = pybind11;

namespace exactextract {
class PyWriter : public OutputWriter
{

  public:
    void write(const Feature& f) override
    {
        // https://github.com/pybind/pybind11/issues/2033#issuecomment-703177186
        py::object dummy = py::cast(f, py::return_value_policy::reference);
        PYBIND11_OVERLOAD_PURE(void, PyWriter, write, f);
    }
};

void
bind_writer(py::module& m)
{
    py::class_<OutputWriter, PyWriter>(m, "Writer")
      .def(py::init<>())
      .def("write", &OutputWriter::write);
}
}
