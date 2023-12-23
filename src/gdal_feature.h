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

#include "ogr_api.h"

#include <geos_c.h>
#include <stdexcept>

namespace exactextract {

class GDALFeature : public Feature
{
  public:
    explicit GDALFeature(OGRFeatureH feature)
      : m_feature(feature)
      , m_context(nullptr)
    {
    }

    GDALFeature(GDALFeature&& other)
      : m_feature(other.m_feature)
      , m_geom(std::move(other.m_geom))
      , m_context(other.m_context)
    {
        other.m_context = nullptr;
        other.m_feature = nullptr;
    }

    GDALFeature& operator=(GDALFeature&& other)
    {
        if (m_feature != nullptr) {
            OGR_F_Destroy(m_feature);
        }

        if (m_context != nullptr) {
            GEOS_finish_r(m_context);
        }

        m_feature = other.m_feature;
        m_geom = std::move(other.m_geom);
        m_context = other.m_context;

        other.m_context = nullptr;
        other.m_feature = nullptr;

        return *this;
    }

    ~GDALFeature() override
    {
        if (m_feature != nullptr) {
            OGR_F_Destroy(m_feature);
        }

        if (m_context != nullptr) {
            GEOS_finish_r(m_context);
        }
    }

    GDALFeature(const GDALFeature& other) = delete;
    GDALFeature& operator=(const GDALFeature& other) = delete;

    const std::type_info& field_type(const std::string& name) const override
    {
        auto pos = field_index(name);
        OGRFieldDefnH defn = OGR_F_GetFieldDefnRef(m_feature, pos);
        auto type = OGR_Fld_GetType(defn);

        if (type == OFTString || type == OFTInteger64) {
            return typeid(std::string);
        } else if (type == OFTReal) {
            return typeid(double);
        } else {
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
    }

    void set(const std::string& name, double value) override
    {
        OGR_F_SetFieldDouble(m_feature, field_index(name), value);
    }

    void set(const std::string& name, std::int32_t value) override
    {
        OGR_F_SetFieldInteger(m_feature, field_index(name), value);
    }

    void set(const std::string& name, std::int64_t value) override
    {
        OGR_F_SetFieldInteger64(m_feature, field_index(name), value);
    }

    void set(const std::string& name, std::size_t value) override
    {
        if (value > std::numeric_limits<std::int64_t>::max()) {
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

    std::int32_t get_int(const std::string& name) const override
    {
        return OGR_F_GetFieldAsInteger(m_feature, field_index(name));
    }

    const GEOSGeometry* geometry() const override
    {
        if (m_geom == nullptr) {
            OGRGeometryH geom = OGR_F_GetGeometryRef(m_feature);

            auto sz = static_cast<size_t>(OGR_G_WkbSize(geom));
            auto buff = std::make_unique<unsigned char[]>(sz);
            OGR_G_ExportToWkb(geom, wkbXDR, buff.get());

            m_context = initGEOS_r(nullptr, nullptr);
            m_geom = geos_ptr(m_context, GEOSGeomFromWKB_buf_r(m_context, buff.get(), sz));
        }

        return m_geom.get();
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
    mutable GEOSContextHandle_t m_context;
};

}
