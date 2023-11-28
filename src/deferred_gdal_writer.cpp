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

#include "deferred_gdal_writer.h"
#include "operation.h"

namespace exactextract {

void
DeferredGDALWriter::write(const Feature& f)
{
    m_features.emplace_back(f);
}

void
DeferredGDALWriter::add_operation(const Operation&) {}

void
DeferredGDALWriter::finish()
{
    std::map<std::string, OGRFieldDefnH> ogr_fields;

    for (const auto& feature : m_features) {
        for (const auto& [field_name, value] : feature.map()) {

            if (ogr_fields.find(field_name) == ogr_fields.end()) {
                OGRFieldType field_type;

                if (value.type() == typeid(std::string)) {
                    field_type = OFTString;
                } else if (value.type() == typeid(double) || value.type() == typeid(float)) {
                    field_type = OFTReal;
                } else if (value.type() == typeid(std::int64_t) || value.type() == typeid(std::uint64_t)) {
                    field_type = OFTInteger64;
                } else if (value.type() == typeid(std::int32_t) || value.type() == typeid(std::uint32_t)) {
                    field_type = OFTInteger;
                }

                ogr_fields[field_name] = OGR_Fld_Create(field_name.c_str(), field_type);
            }
        }
    }

    constexpr bool approx_ok = true;

    for (const auto& [field_name, field_defn] : ogr_fields) {
        OGRFeatureDefnH defn = OGR_L_GetLayerDefn(m_layer);

        if (OGR_FD_GetFieldIndex(defn, field_name.c_str()) == -1) {
            OGR_L_CreateField(m_layer, field_defn, approx_ok);
        }
    }

    OGRFeatureDefnH defn = OGR_L_GetLayerDefn(m_layer);
    for (const auto& feature : m_features) {
        OGRFeatureH f = OGR_F_Create(defn);

        for (const auto& [field_name, value] : feature.map()) {
            auto pos = OGR_F_GetFieldIndex(f, field_name.c_str());

            if (pos == -1) {
                throw std::runtime_error("Unexpected field: " + field_name);
            }

            if (value.type() == typeid(std::string)) {
                OGR_F_SetFieldString(f, pos, std::any_cast<std::string>(value).c_str());
            } else if (value.type() == typeid(float)) {
                OGR_F_SetFieldDouble(f, pos, static_cast<double>(std::any_cast<float>(value)));
            } else if (value.type() == typeid(double)) {
                OGR_F_SetFieldDouble(f, pos, std::any_cast<double>(value));
            } else if (value.type() == typeid(std::int64_t)) {
                OGR_F_SetFieldInteger64(f, pos, std::any_cast<std::int64_t>(value));
            } else if (value.type() == typeid(std::uint64_t)) {
                auto intval = std::any_cast<std::uint64_t>(value);
                if (intval > std::numeric_limits<std::int64_t>::max()) {
                    throw std::runtime_error("Unhandled int64 value.");
                }
                OGR_F_SetFieldInteger64(f, pos, static_cast<std::int64_t>(intval));
            } else {
                throw std::runtime_error("Unhandled type: " + std::string(value.type().name()));
            }
        }

        if (OGR_L_CreateFeature(m_layer, f) != OGRERR_NONE) {
            throw std::runtime_error("Error writing feature.");
        }
        OGR_F_Destroy(f);
    }
}

}
