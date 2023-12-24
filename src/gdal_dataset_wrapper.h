// Copyright (c) 2018-2024 ISciences, LLC.
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

#include "feature_source.h"
#include "gdal_feature.h"

#include <gdal.h>
#include <string>

namespace exactextract {

class Feature;

class GDALDatasetWrapper : public FeatureSource
{
  public:
    GDALDatasetWrapper(const std::string& filename, const std::string& layer);

    GDALDatasetWrapper(GDALDatasetWrapper&&) noexcept;

    GDALDatasetWrapper& operator=(GDALDatasetWrapper&&) noexcept;

    const Feature& feature() const override;

    bool next() override;

    void copy_field(const std::string& field_name, OGRLayerH to) const;

    void set_select(const std::vector<std::string>& cols);

    ~GDALDatasetWrapper() override;

  private:
    GDALDatasetH m_dataset;
    OGRLayerH m_layer;
    GDALFeature m_feature;
    bool m_layer_is_sql;
};

}
