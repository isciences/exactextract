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

#include "gdal_writer.h"
#include "map_feature.h"
#include "ogr_api.h"

#include <map>
#include <string>

namespace exactextract {

class DeferredGDALWriter : public GDALWriter
{
  public:
    using GDALWriter::GDALWriter;

    void add_operation(const Operation& op) override;

    std::unique_ptr<Feature> create_feature() override;

    void write(const Feature& f) override;

    void finish() override;

  private:
    std::map<std::string, OGRFieldDefnH> m_fields;
    std::vector<MapFeature> m_features;
};
}
