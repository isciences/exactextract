// Copyright (c) 2019-2024 ISciences, LLC.
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

#include "output_writer.h"
#include <gdal.h>

namespace exactextract {

class GDALDatasetWrapper;

class GDALWriter : public OutputWriter
{

  public:
    explicit GDALWriter(const std::string& filename, bool unnest, OGRSpatialReferenceH = nullptr);

    ~GDALWriter() override;

    static std::string get_driver_name(const std::string& filename);

    void add_operation(const Operation& op) override;

    std::unique_ptr<Feature> create_feature() override;

    void write(const Feature& f) override;

    void copy_field(const GDALDatasetWrapper& w, const std::string& field_name);

    static OGRFieldType ogr_type(const std::type_info&, bool unnest);

  protected:
    GDALDatasetH m_dataset;
    OGRLayerH m_layer;

    bool m_unnest_if_needed = true;
    bool m_contains_nested_fields = false;
};

}
