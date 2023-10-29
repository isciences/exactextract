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

#include <any>
#include <unordered_map>

namespace exactextract {

class MapFeature : public Feature
{

  public:
    void set(const std::string& name, double value) override
    {
        m_map[name] = value;
    }

    void set(const std::string& name, float value) override
    {
        m_map[name] = value;
    }

    void set(const std::string& name, std::size_t value) override
    {
        m_map[name] = value;
    }

    void set(const std::string& name, std::string value) override
    {
        m_map[name] = std::move(value);
    }

    const std::unordered_map<std::string, std::any>& map() const
    {
        return m_map;
    }

    template<typename T>
    T get(const std::string& field) const {
        return std::any_cast<T>(m_map.at(field));
    }

  private:
    std::unordered_map<std::string, std::any> m_map;
};

}
