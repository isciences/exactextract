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

#include "feature.h"
#include "geos_utils.h"

#include "ogr_api.h"

#include <geos_c.h>
#include <stdexcept>

namespace exactextract {

class GDALFeature : public Feature
{
  public:
    explicit GDALFeature(OGRFeatureH feature)
      : m_feature(feature)
    {
    }

    GDALFeature(GDALFeature&& other) noexcept
      : m_feature(other.m_feature)
      , m_geom(std::move(other.m_geom))
    {
        other.m_feature = nullptr;
    }

    GDALFeature& operator=(GDALFeature&& other) noexcept
    {
        if (m_feature != nullptr) {
            OGR_F_Destroy(m_feature);
        }

        m_feature = other.m_feature;
        m_geom = std::move(other.m_geom);

        other.m_feature = nullptr;

        return *this;
    }

    ~GDALFeature() override
    {
        if (m_feature != nullptr) {
            OGR_F_Destroy(m_feature);
        }
    }

    GDALFeature(const GDALFeature& other) = delete;
    GDALFeature& operator=(const GDALFeature& other) = delete;

    ValueType field_type(const std::string& name) const override
    {
        auto pos = field_index(name);
        OGRFieldDefnH defn = OGR_F_GetFieldDefnRef(m_feature, pos);

        switch (OGR_Fld_GetType(defn)) {
            case OFTString:
                return ValueType::STRING;
            case OFTInteger:
                return ValueType::INT;
            case OFTIntegerList:
                return ValueType::INT_ARRAY;
            case OFTInteger64:
                return ValueType::INT64;
            case OFTInteger64List:
                return ValueType::INT64_ARRAY;
            case OFTReal:
                return ValueType::DOUBLE;
            case OFTRealList:
                return ValueType::DOUBLE_ARRAY;
            default:
                throw std::runtime_error("Unhandled type.");
        }
    }

    void copy_to(Feature& dst) const override
    {
        int n = OGR_F_GetFieldCount(m_feature);

        for (int i = 0; i < n; i++) {
            OGRFieldDefnH defn = OGR_F_GetFieldDefnRef(m_feature, i);
            std::string name = OGR_Fld_GetNameRef(defn);
            dst.set(name, *this);
        }

        dst.set_geometry(geometry());
    }

    using Feature::set;

    void set(const std::string& name, const DoubleArray& value) override
    {
        OGR_F_SetFieldDoubleList(m_feature, field_index(name), static_cast<int>(value.size), value.data);
    }

    void set(const std::string& name, double value) override
    {
        OGR_F_SetFieldDouble(m_feature, field_index(name), value);
    }

    void set(const std::string& name, const IntegerArray& value) override
    {
        OGR_F_SetFieldIntegerList(m_feature, field_index(name), static_cast<int>(value.size), value.data);
    }

    void set(const std::string& name, std::int32_t value) override
    {
        OGR_F_SetFieldInteger(m_feature, field_index(name), value);
    }

    void set(const std::string& name, std::int64_t value) override
    {
        OGR_F_SetFieldInteger64(m_feature, field_index(name), value);
    }

    void set(const std::string& name, const Integer64Array& value) override
    {
        OGR_F_SetFieldInteger64List(m_feature, field_index(name), static_cast<int>(value.size), reinterpret_cast<const GIntBig*>(value.data));
    }

    void set(const std::string& name, std::size_t value) override
    {
        if (value > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
            throw std::runtime_error("Value too large to write");
        }
        OGR_F_SetFieldInteger64(m_feature, field_index(name), static_cast<std::int64_t>(value));
    }

    void set(const std::string& name, std::string value) override
    {
        OGR_F_SetFieldString(m_feature, field_index(name), value.c_str());
    }

    std::string get_string(const std::string& name) const override
    {
        return std::string(OGR_F_GetFieldAsString(m_feature, field_index(name)));
    }

    double get_double(const std::string& name) const override
    {
        return OGR_F_GetFieldAsDouble(m_feature, field_index(name));
    }

    DoubleArray get_double_array(const std::string& name) const override
    {
        int size;
        const double* arr = OGR_F_GetFieldAsDoubleList(m_feature, field_index(name), &size);
        return { arr, static_cast<std::size_t>(size) };
    }

    std::int32_t get_int(const std::string& name) const override
    {
        return OGR_F_GetFieldAsInteger(m_feature, field_index(name));
    }

    IntegerArray get_integer_array(const std::string& name) const override
    {
        int size;
        const std::int32_t* arr = OGR_F_GetFieldAsIntegerList(m_feature, field_index(name), &size);
        return { arr, static_cast<std::size_t>(size) };
    }

    std::int64_t get_int64(const std::string& name) const override
    {
        return OGR_F_GetFieldAsInteger64(m_feature, field_index(name));
    }

    Integer64Array get_integer64_array(const std::string& name) const override
    {
        int size;
        const std::int64_t* arr = reinterpret_cast<const std::int64_t*>(OGR_F_GetFieldAsInteger64List(m_feature, field_index(name), &size));
        return { arr, static_cast<std::size_t>(size) };
    }

    const GEOSGeometry* geometry() const override
    {
        if (m_geom == nullptr) {
            OGRGeometryH geom = OGR_F_GetGeometryRef(m_feature);

            if (geom != nullptr) {
                auto sz = static_cast<size_t>(OGR_G_WkbSize(geom));
                auto buff = std::make_unique<unsigned char[]>(sz);
                OGR_G_ExportToWkb(geom, wkbXDR, buff.get());

                m_geom = geos_ptr(m_context, GEOSGeomFromWKB_buf_r(m_context, buff.get(), sz));
            }
        }

        return m_geom.get();
    }

    void set_geometry(const GEOSGeometry* geom) override
    {
        if (geom == nullptr) {
            m_geom.reset();
            OGR_F_SetGeometry(m_feature, nullptr);
        } else {
            m_geom = geos_ptr(m_context, GEOSGeom_clone_r(m_context, geom));

            std::size_t wkbSize;
            unsigned char* wkb = GEOSGeomToWKB_buf_r(m_context, m_geom.get(), &wkbSize);
            OGRGeometryH ogr_geom;
            OGR_G_CreateFromWkb(wkb, nullptr, &ogr_geom, -1);
            OGR_F_SetGeometryDirectly(m_feature, ogr_geom);
        }
    }

    OGRFeatureH raw() const
    {
        return m_feature;
    }

  private:
    int field_index(const std::string& name) const
    {
        auto ret = OGR_F_GetFieldIndex(m_feature, name.c_str());
        if (ret == -1) {
            throw std::runtime_error("Unexpected field name: " + name);
        }
        return ret;
    }

    OGRFeatureH m_feature;
    mutable geom_ptr_r m_geom;
    static inline GEOSContextHandle_t m_context = initGEOS_r(nullptr, nullptr);
};

}
