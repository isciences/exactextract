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
#include "gdal_feature.h"
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

                if (std::holds_alternative<std::string>(value)) {
                    field_type = OFTString;
                } else if (std::holds_alternative<double>(value)) {
                    field_type = OFTReal;
                } else if (std::holds_alternative<std::int32_t>(value)) {
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
        GDALFeature f(OGR_F_Create(defn));
        feature.copy_to(f);

        if (OGR_L_CreateFeature(m_layer, f.raw()) != OGRERR_NONE) {
            throw std::runtime_error("Error writing feature.");
        }
    }
}

}
