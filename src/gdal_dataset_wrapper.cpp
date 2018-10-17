// Copyright (c) 2018 ISciences, LLC.
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

#include "gdal_dataset_wrapper.h"

namespace exactextract {

    GDALDatasetWrapper::GDALDatasetWrapper(const std::string &filename, int layer) {
        m_dataset = GDALOpenEx(filename.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
        if (m_dataset == nullptr) {
            throw std::runtime_error("Failed to open " + filename);
        }

        m_layer = GDALDatasetGetLayer(m_dataset, layer);
        OGR_L_ResetReading(m_layer);
        m_feature = nullptr;
    }

    bool GDALDatasetWrapper::next() {
        if (m_feature != nullptr) {
            OGR_F_Destroy(m_feature);
        }
        m_feature = OGR_L_GetNextFeature(m_layer);
        return m_feature != nullptr;
    }

    GEOSGeometry* GDALDatasetWrapper::feature_geometry(const GEOSContextHandle_t &geos_context) const {
        OGRGeometryH geom = OGR_F_GetGeometryRef(m_feature);

        int sz = OGR_G_WkbSize(geom);
        auto buff = std::make_unique<unsigned char[]>(sz);
        OGR_G_ExportToWkb(geom, wkbXDR, buff.get());

        return GEOSGeomFromWKB_buf(buff.get(), sz);
    }

    std::string GDALDatasetWrapper::feature_field(const std::string &field_name) const {
            int index = OGR_F_GetFieldIndex(m_feature, field_name.c_str());
            // TODO check handling of invalid field name
            return OGR_F_GetFieldAsString(m_feature, index);
    }

    GDALDatasetWrapper::~GDALDatasetWrapper(){
            GDALClose(m_dataset);

            if (m_feature != nullptr) {
                    OGR_F_Destroy(m_feature);
            }
    }
}
