// Copyright (c) 2024 ISciences, LLC.
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
#include <gdal.h>
#include <stdexcept>
#include <vector>

namespace exactextract {
class GDALFeatureUnnester
{
  public:
    GDALFeatureUnnester(const Feature& f, OGRFeatureDefnH defn)
      : m_feature(f)
      , m_defn(defn)
    {
    }

    void initialize_features(std::size_t n)
    {
        if (m_features.size() == n || (n == 1 && !m_features.empty())) {
            return; // alrady initialized
        }

        if (m_features.empty()) {
            for (std::size_t i = 0; i < n; i++) {
                m_features.emplace_back(make_feature());
            }
        } else if (m_features.size() == 1) {
            for (std::size_t i = 1; i < n; i++) {
                m_features.emplace_back(make_feature());
                m_features.front()->copy_to(*m_features.back());
            }
        } else {
            throw std::runtime_error("Cannot unnest feature whose properties have different array lengths.");
        }
    }

    std::unique_ptr<GDALFeature> make_feature()
    {
        return std::make_unique<GDALFeature>(OGR_F_Create(m_defn));
    }

    void unnest()
    {
        for (int i = 0; i < OGR_FD_GetFieldCount(m_defn); i++) {
            OGRFieldDefnH field = OGR_FD_GetFieldDefn(m_defn, i);
            const char* field_name = OGR_Fld_GetNameRef(field);

            try {
                auto&& field_value = m_feature.get(field_name);

                std::visit([&field_name, this](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;

                    if constexpr (std::is_same_v<T, Feature::IntegerArray> ||
                                  std::is_same_v<T, Feature::Integer64Array> ||
                                  std::is_same_v<T, Feature::DoubleArray>) {
                        if (arg.size == 0) {
                            // No features emitted when we have an empty array
                            m_features.clear();
                            return;
                        }

                        initialize_features(arg.size);
                        for (std::size_t i = 0; i < arg.size; i++) {
                            m_features[i]->set(field_name, arg.data[i]);
                        }
                    } else {
                        initialize_features(1);
                        for (auto& f : m_features) {
                            f->set(field_name, arg);
                        }
                    }
                },
                           field_value);
            } catch (std::out_of_range) {
                // Skip
                // TODO eliminate the try/catch with a Feature::has_field or Feature::get_if?
            }
        }
    }

    const std::vector<std::unique_ptr<GDALFeature>>& features() const
    {
        return m_features;
    }

  private:
    const Feature& m_feature;
    OGRFeatureDefnH m_defn;
    std::vector<std::unique_ptr<GDALFeature>> m_features;
};

}