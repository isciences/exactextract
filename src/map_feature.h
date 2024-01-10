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

#pragma once

#include "feature.h"
#include "geos_utils.h"

#include <unordered_map>

namespace exactextract {
class MapFeature : public Feature
{
  public:
    MapFeature()
    {
    }

    virtual ~MapFeature()
    {
    }

    explicit MapFeature(const Feature& other)
    {
        other.copy_to(*this);
        if (other.geometry() != nullptr) {
            m_geom = geos_ptr(m_geos_context, GEOSGeom_clone_r(m_geos_context, other.geometry()));
        }
    }

    MapFeature(MapFeature&& other) = default;

    MapFeature& operator=(MapFeature&& other) = default;

    const std::type_info& field_type(const std::string& name) const override
    {
        const FieldValue& val = m_map.at(name);
        // https://stackoverflow.com/a/53697591/2171894
        return std::visit([](auto&& x) -> decltype(auto) { return typeid(x); }, val);
    }

    void set(const std::string& name, double value) override
    {
        m_map[name] = value;
    }

    void set(const std::string& name, const DoubleArray& value) override
    {
        m_map[name] = value;
    }

    DoubleArray get_double_array(const std::string& name) const override
    {
        return get<DoubleArray>(name);
    }

    void set(const std::string& name, std::int32_t value) override
    {
        m_map[name] = value;
    }

    void set(const std::string& name, const IntegerArray& value) override
    {
        m_map[name] = value;
    }

    IntegerArray get_integer_array(const std::string& name) const override
    {
        return get<IntegerArray>(name);
    }

    void set(const std::string& name, const Integer64Array& value) override
    {
        m_map[name] = value;
    }

    Integer64Array get_integer64_array(const std::string& name) const override
    {
        return get<Integer64Array>(name);
    }

    void set(const std::string& name, std::string value) override
    {
        m_map[name] = std::move(value);
    }

    void copy_to(Feature& dst) const override
    {
        for (const auto& [k, v] : m_map) {
            dst.set(k, *this);
        }
        dst.set_geometry(geometry());
    }

    void set_geometry(geom_ptr_r geom)
    {
        m_geom = std::move(geom);
    }

    void set_geometry(const GEOSGeometry* g) override
    {
        m_geom = g ? geos_ptr(m_geos_context, GEOSGeom_clone_r(m_geos_context, g)) : nullptr;
    }

    const GEOSGeometry* geometry() const override
    {
        return m_geom.get();
    }

    const std::unordered_map<std::string, FieldValue>& map() const
    {
        return m_map;
    }

    template<typename T>
    T get(const std::string& field) const
    {
        return std::get<T>(m_map.at(field));
    }

    std::string get_string(const std::string& name) const override
    {
        return get<std::string>(name);
    }

    double get_double(const std::string& name) const override
    {
        return get<double>(name);
    }

    std::int32_t get_int(const std::string& name) const override
    {
        return get<std::int32_t>(name);
    }

  private:
    inline static GEOSContextHandle_t m_geos_context = initGEOS_r(nullptr, nullptr);

    std::unordered_map<std::string, FieldValue> m_map;
    geom_ptr_r m_geom;
};
}
