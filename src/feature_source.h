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

#include <geos_c.h>
#include <string>

namespace exactextract {

class FeatureSource
{

  public:
    virtual bool next() = 0;

    virtual GEOSGeometry* feature_geometry(const GEOSContextHandle_t&) const = 0;

    virtual std::string feature_field(const std::string& field_name) const = 0;

    virtual const std::string& id_field() const = 0;
};

}
