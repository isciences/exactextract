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
#include <pybind11/stl.h>

#include "feature_sequential_processor.h"
#include "feature_source.h"
#include "output_writer.h"
#include "processor.h"
#include "processor_bindings.h"
#include "raster_sequential_processor.h"

namespace py = pybind11;

namespace exactextract {
void
bind_processor(py::module& m)
{
    py::class_<Processor>(m, "Processor")
      .def("add_operation", &Processor::add_operation)
      .def("add_col", &Processor::include_col)
      .def("add_geom", &Processor::include_geometry)
      .def("process", &Processor::process)
      .def("set_max_cells_in_memory", &Processor::set_max_cells_in_memory, py::arg("n"))
      .def("show_progress", &Processor::show_progress, py::arg("val"));

    py::class_<FeatureSequentialProcessor, Processor>(m, "FeatureSequentialProcessor")
      .def(py::init<FeatureSource&, OutputWriter&>());

    py::class_<RasterSequentialProcessor, Processor>(m, "RasterSequentialProcessor")
      .def(py::init<FeatureSource&, OutputWriter&>());
}
}
