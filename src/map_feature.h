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

#include <any>
#include <unordered_map>

namespace exactextract {

class MapFeature : public Feature
{

  public:
    MapFeature() {}

    virtual ~MapFeature() {}

    explicit MapFeature (const Feature& other) {
        other.copy_to(*this);
        if (other.geometry() != nullptr) {
            m_geom = geos_ptr(m_geos_context, GEOSGeom_clone_r(m_geos_context, other.geometry()));
        }
    }

    explicit MapFeature (MapFeature && other) = default;
    MapFeature& operator=(MapFeature&& other) = default;

    const std::type_info& field_type(const std::string& name) const override
    {
        return m_map.at(name).type();
    }

    void set(const std::string& name, double value) override
    {
        m_map[name] = value;
    }

    void set(const std::string& name, float value) override
    {
        m_map[name] = value;
    }

    void set(const std::string& name, std::size_t value) override
    {
        m_map[name] = value;
    }

    void set(const std::string& name, std::string value) override
    {
        m_map[name] = std::move(value);
    }

    void set(const std::string& name, const Feature& f) override {
        const auto& type = f.field_type(name);

        if (type == typeid(std::string)) {
            m_map[name] = f.get_string(name);
        } else if (type == typeid(double)) {
            m_map[name] = f.get_double(name);
        } else if (type == typeid(float)) {
            m_map[name] = f.get_float(name);
        } else {
            throw std::runtime_error("Unhandled type: " + std::string(type.name()));
        }
    }

    void copy_to(Feature& dst) const override {
        for (const auto& [k, v] : m_map) {
            dst.set(k, *this);
        }
    }

    void set_geometry(geom_ptr_r geom) {
        m_geom = std::move(geom);
    }

    const GEOSGeometry* geometry() const override {
        return m_geom.get();
    }

    const std::unordered_map<std::string, std::any>& map() const
    {
        return m_map;
    }

    template<typename T>
    T get(const std::string& field) const {
        return std::any_cast<T>(m_map.at(field));
    }

    std::string get_string(const std::string& name) const override {
        return get<std::string>(name);
    }

    double get_double(const std::string& name) const override {
        return get<double>(name);
    }

    float get_float(const std::string& name) const override {
        return get<float>(name);
    }

  private:
    inline static GEOSContextHandle_t m_geos_context = initGEOS_r(nullptr, nullptr);

    std::unordered_map<std::string, std::any> m_map;
    geom_ptr_r m_geom;
};

}
