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

#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <variant>
#include <vector>

#include <geos_c.h>

namespace exactextract {
class Feature
{
  public:
    template<typename T>
    struct Array
    {
        using value_type = T;

        explicit Array(std::size_t p_size)
          : data(new T[p_size])
          , size(p_size)
          , owned(true)
        {
        }

        Array(const T* p_data, std::size_t p_size)
          : data(p_data)
          , size(p_size)
          , owned(false)
        {
        }

        Array(const Array& other)
        {
            data = new T[other.size];
            size = other.size;
            owned = true;
            std::memcpy((void*)data, other.data, size * sizeof(T));
        }

        Array& operator=(const Array& other)
        {
            if (&other == this) {
                return *this;
            }

            if (owned) {
                delete[] (data);
            }
            data = new T[other.size];
            size = other.size;
            owned = true;
            std::memcpy((void*)data, other.data, size * sizeof(T));

            return *this;
        }

        ~Array()
        {
            if (owned) {
                delete[] data;
            }
        }

        const T* data;
        std::size_t size;
        bool owned;
    };

    using FloatArray = Array<float>;
    using DoubleArray = Array<double>;
    using IntegerArray = Array<std::int32_t>;
    using Integer64Array = Array<std::int64_t>;
    using FieldValue = std::variant<std::string, double, std::int32_t, std::int64_t, DoubleArray, IntegerArray, Integer64Array>;

    enum class ValueType
    {
        STRING,
        DOUBLE,
        DOUBLE_ARRAY,
        INT,
        INT64,
        INT_ARRAY,
        INT64_ARRAY
    };

    virtual ~Feature()
    {
    }

    struct FieldTypeGetter
    {
        constexpr ValueType operator()(double) const
        {
            return ValueType::DOUBLE;
        }
        constexpr ValueType operator()(std::int32_t) const
        {
            return ValueType::INT;
        }
        constexpr ValueType operator()(std::size_t) const
        {
            return ValueType::INT64;
        }
        constexpr ValueType operator()(std::int64_t) const
        {
            return ValueType::INT64;
        }
        constexpr ValueType operator()(const std::string&) const
        {
            return ValueType::STRING;
        }
        constexpr ValueType operator()(const DoubleArray&) const
        {
            return ValueType::DOUBLE_ARRAY;
        }
        constexpr ValueType operator()(const std::vector<double>&) const
        {
            return ValueType::DOUBLE_ARRAY;
        }
        constexpr ValueType operator()(const std::vector<float>&) const
        {
            return ValueType::DOUBLE_ARRAY;
        }
        constexpr ValueType operator()(const std::vector<std::int8_t>&) const
        {
            return ValueType::INT_ARRAY;
        }
        constexpr ValueType operator()(const std::vector<std::int16_t>&) const
        {
            return ValueType::INT_ARRAY;
        }
        constexpr ValueType operator()(const std::vector<std::int32_t>&) const
        {
            return ValueType::INT_ARRAY;
        }
        constexpr ValueType operator()(const IntegerArray&) const
        {
            return ValueType::INT_ARRAY;
        }
        constexpr ValueType operator()(const Integer64Array&) const
        {
            return ValueType::INT64_ARRAY;
        }
        constexpr ValueType operator()(const std::vector<std::int64_t>&) const
        {
            return ValueType::INT64_ARRAY;
        }
    };

    // Interface methods
    virtual ValueType field_type(const std::string& name) const = 0;

    virtual void set(const std::string& name, std::string value) = 0;

    virtual void set(const std::string& name, double value) = 0;

    virtual void set(const std::string& name, std::int32_t value) = 0;

    virtual void set(const std::string& name, std::int64_t value) = 0;

    virtual void set(const std::string& name, const DoubleArray& value) = 0;

    virtual void set(const std::string& name, const IntegerArray& value) = 0;

    virtual void set(const std::string& name, const Integer64Array& value) = 0;

    virtual std::string get_string(const std::string& name) const = 0;

    virtual double get_double(const std::string& name) const = 0;

    virtual DoubleArray get_double_array(const std::string& name) const = 0;

    virtual std::int32_t get_int(const std::string& name) const = 0;

    virtual std::int64_t get_int64(const std::string& name) const = 0;

    virtual IntegerArray get_integer_array(const std::string& name) const = 0;

    virtual Integer64Array get_integer64_array(const std::string& name) const = 0;

    virtual void copy_to(Feature& dst) const = 0;

    virtual const GEOSGeometry* geometry() const = 0;

    virtual void set_geometry(const GEOSGeometry*) = 0;

    // Convenience methods (can be overridden)
    virtual void set(const std::string& name, float value);

    virtual void set(const std::string& name, const FloatArray& value);

    virtual void set(const std::string& name, const std::vector<std::int8_t>& value);

    virtual void set(const std::string& name, const std::vector<std::int16_t>& value);

    virtual void set(const std::string& name, const std::vector<std::int32_t>& value);

    virtual void set(const std::string& name, const std::vector<std::int64_t>& value);

    virtual void set(const std::string& name, const std::vector<float>& value);

    virtual void set(const std::string& name, const std::vector<double>& value);

    virtual void set(const std::string& name, std::size_t value);

    virtual void set(const std::string& name, const Feature& other);

    virtual FieldValue get(const std::string& name) const;
};
}
