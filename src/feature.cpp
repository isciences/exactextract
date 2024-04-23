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
    switch (f.field_type(name)) {
        case ValueType::STRING:
            set(name, f.get_string(name));
            return;
        case ValueType::DOUBLE:
            set(name, f.get_double(name));
            return;
        case ValueType::DOUBLE_ARRAY:
            set(name, f.get_double_array(name));
            return;
        case ValueType::INT:
            set(name, f.get_int(name));
            return;
        case ValueType::INT64:
            set(name, f.get_int64(name));
            return;
        case ValueType::INT_ARRAY:
            set(name, f.get_integer_array(name));
            return;
        case ValueType::INT64_ARRAY:
            set(name, f.get_integer64_array(name));
            return;
    }
}

void
Feature::set(const std::string& name, std::uint32_t value)
{
    set(name, static_cast<std::int64_t>(value));
}

void
Feature::set(const std::string& name, std::uint64_t value)
{
    if (value > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
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

template<typename T_out, typename T_in>
void
copy_and_set_vect(Feature& f, const std::string& name, const std::vector<T_in>& value)
{
    std::vector<T_out> new_values(value.size());
    for (std::size_t i = 0; i < value.size(); i++) {
        if constexpr (std::numeric_limits<T_in>::max() > std::numeric_limits<T_out>::max()) {
            if (value[i] > std::numeric_limits<T_out>::max()) {
                throw std::runtime_error("Array value too large.");
            }
        }
        new_values[i] = value[i];
    }
    f.set(name, new_values);
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
Feature::set(const std::string& name, const std::vector<std::int8_t>& value)
{
    copy_and_set_vect<std::int32_t>(*this, name, value);
}

void
Feature::set(const std::string& name, const std::vector<std::uint8_t>& value)
{
    copy_and_set_vect<std::int32_t>(*this, name, value);
}

void
Feature::set(const std::string& name, const std::vector<std::int16_t>& value)
{
    copy_and_set_vect<std::int32_t>(*this, name, value);
}

void
Feature::set(const std::string& name, const std::vector<std::uint16_t>& value)
{
    copy_and_set_vect<std::int32_t>(*this, name, value);
}

void
Feature::set(const std::string& name, const std::vector<std::uint32_t>& value)
{
    copy_and_set_vect<std::int64_t>(*this, name, value);
}

void
Feature::set(const std::string& name, const std::vector<std::uint64_t>& value)
{
    copy_and_set_vect<std::int64_t>(*this, name, value);
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
    switch (field_type(name)) {
        case ValueType::STRING:
            return get_string(name);
        case ValueType::DOUBLE:
            return get_double(name);
        case ValueType::DOUBLE_ARRAY:
            return get_double_array(name);
        case ValueType::INT:
            return get_int(name);
        case ValueType::INT64:
            return get_int64(name);
        case ValueType::INT_ARRAY:
            return get_integer_array(name);
        case ValueType::INT64_ARRAY:
            return get_integer64_array(name);
    }

    throw std::runtime_error("Unsupported type in Feature::get");
}

}
