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

#include <string>
#include <typeinfo>

#include <geos_c.h>

#pragma once

namespace exactextract {

class Feature
{
  public:
    virtual ~Feature() {}

    virtual void set(const std::string& name, double value) = 0;
    virtual void set(const std::string& name, float value) = 0;
    virtual void set(const std::string& name, std::size_t value) = 0;
    virtual void set(const std::string& name, std::string value) = 0;

    virtual const std::type_info& field_type(const std::string& name) const = 0;

    virtual void set(const std::string& name, const Feature& other) = 0;

    virtual std::string get_string(const std::string& name) const = 0;
    virtual double get_double(const std::string& name) const = 0;
    virtual float get_float(const std::string& name) const = 0;

    virtual void copy_to(Feature& dst) const = 0;

    virtual const GEOSGeometry* geometry() const = 0;
};

}

