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

#include "ogr_api.h"

#include <stdexcept>

namespace exactextract {

class GDALFeature : public Feature
{
  public:
    explicit GDALFeature(OGRFeatureH feature)
      : m_feature(feature)
    {
    }

    void set(const std::string& name, double value) override
    {
        OGR_F_SetFieldDouble(m_feature, field_index(name), value);
    }

    void set(const std::string& name, float value) override
    {
        OGR_F_SetFieldDouble(m_feature, field_index(name), static_cast<double>(value));
    }

    void set(const std::string& name, std::size_t value) override
    {
        if (value > std::numeric_limits<std::int64_t>::max()) {
            throw std::runtime_error("Value too large to write");
        }
        OGR_F_SetFieldInteger64(m_feature, field_index(name), static_cast<std::int64_t>(value));
    }

  private:
    int field_index(const std::string& name)
    {
        auto ret = OGR_F_GetFieldIndex(m_feature, name.c_str());
        if (ret == -1) {
            throw std::runtime_error("Unexpected field name");
        }
        return ret;
    }

    OGRFeatureH m_feature;
};

}
