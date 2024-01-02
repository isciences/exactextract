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

#include "feature.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace exactextract {
void
Feature::set(const std::string& name, const Feature& f)
{
    const auto& type = f.field_type(name);

    if (type == typeid(std::string)) {
        set(name, f.get_string(name));
    } else if (type == typeid(double)) {
        set(name, f.get_double(name));
    } else if (type == typeid(DoubleArray)) {
        set(name, f.get_double_array(name));
    } else if (type == typeid(std::int8_t)) {
        set(name, f.get_int(name));
    } else if (type == typeid(std::int16_t)) {
        set(name, f.get_int(name));
    } else if (type == typeid(std::int32_t)) {
        set(name, f.get_int(name));
    } else if (type == typeid(IntegerArray)) {
        set(name, f.get_integer_array(name));
    } else if (type == typeid(std::int64_t)) {
        set(name, f.get_int(name));
    } else if (type == typeid(Integer64Array)) {
        set(name, f.get_integer64_array(name));
    } else if (type == typeid(std::size_t)) {
        set(name, f.get_int(name));
    } else {
        throw std::runtime_error("Unhandled type: " + std::string(type.name()));
    }
}

void
Feature::set(const std::string& name, std::int64_t value)
{
    if (value > std::numeric_limits<std::int32_t>::max() || value < std::numeric_limits<std::int32_t>::min()) {
        throw std::runtime_error("Value is too small/large to store as 32-bit integer.");
    }
    set(name, static_cast<std::int32_t>(value));
}

void
Feature::set(const std::string& name, std::size_t value)
{
    if (value > std::numeric_limits<std::int64_t>::max()) {
        throw std::runtime_error("Value is too large to store as 64-bit integer.");
    }
    set(name, static_cast<std::int64_t>(value));
}

void
Feature::set(const std::string& name, float value)
{
    if (std::isnan(value)) {
        set(name, std::numeric_limits<double>::quiet_NaN());
    } else {
        set(name, static_cast<double>(value));
    }
}

void
Feature::set(const std::string& name, const std::vector<float>& value)
{
    set(name, { value.data(), value.size() });
}

void
Feature::set(const std::string& name, const std::vector<std::int64_t>& value)
{
    set(name, { value.data(), value.size() });
}

void
Feature::set(const std::string& name, const std::vector<std::int32_t>& value)
{
    set(name, { value.data(), value.size() });
}

void
Feature::set(const std::string& name, const std::vector<std::int16_t>& value)
{
    std::vector<std::int32_t> int32_values(value.size());
    for (std::size_t i = 0; i < value.size(); i++) {
        int32_values[i] = value[i];
    }
    set(name, int32_values);
}

void
Feature::set(const std::string& name, const std::vector<std::int8_t>& value)
{
    std::vector<std::int32_t> int32_values(value.size());
    for (std::size_t i = 0; i < value.size(); i++) {
        int32_values[i] = value[i];
    }
    set(name, int32_values);
}

void
Feature::set(const std::string& name, const FloatArray& value)
{
    std::vector<double> dbl_values(value.size);
    for (std::size_t i = 0; i < value.size; i++) {
        dbl_values[i] = static_cast<double>(value.data[i]);
    }
    set(name, dbl_values);
}

void
Feature::set(const std::string& name, const std::vector<double>& value)
{
    set(name, { value.data(), value.size() });
}

Feature::FieldValue
Feature::get(const std::string& name) const
{
    const auto& type = field_type(name);

    if (type == typeid(std::string)) {
        return get_string(name);
    } else if (type == typeid(double)) {
        return get_double(name);
    } else if (type == typeid(std::int32_t)) {
        return get_int(name);
    } else if (type == typeid(IntegerArray)) {
        return get_integer_array(name);
    } else if (type == typeid(Integer64Array)) {
        return get_integer64_array(name);
    } else if (type == typeid(DoubleArray)) {
        return get_double_array(name);
    } else {
        throw std::runtime_error("Unhandled type: " + std::string(type.name()));
    }
}
}
