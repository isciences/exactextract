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

#include "feature_bindings.h"
#include "feature_source_bindings.h"
#include "operation_bindings.h"
#include "processor_bindings.h"
#include "raster_source_bindings.h"
#include "writer_bindings.h"

namespace py = pybind11;

namespace exactextract {
PYBIND11_MODULE(_exactextract, m)
{
    bind_feature(m);
    bind_feature_source(m);
    bind_raster_source(m);
    bind_operation(m);
    bind_processor(m);
    bind_writer(m);
}
}
