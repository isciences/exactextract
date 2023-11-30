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

#include "feature.h"

#include <stdexcept>

namespace exactextract {

void
Feature::set(const std::string& name, const Feature& f)
{
    const auto& type = f.field_type(name);

    if (type == typeid(std::string)) {
        set(name, f.get_string(name));
    } else if (type == typeid(double)) {
        set(name, f.get_double(name));
    } else if (type == typeid(float)) {
        set(name, f.get_float(name));
    } else if (type == typeid(std::int32_t)) {
        set(name, f.get_int(name));
    } else {
        throw std::runtime_error("Unhandled type: " + std::string(type.name()));
    }
}

}
