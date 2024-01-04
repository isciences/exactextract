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

#include "gdal_dataset_wrapper.h"
#include "gdal_feature.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <stdexcept>

#include "utils.h"

namespace exactextract {
const Feature&
GDALDatasetWrapper::feature() const
{
    return m_feature;
}

GDALDatasetWrapper::GDALDatasetWrapper(const std::string& filename, const std::string& layer)
  : m_feature(nullptr)
{
    m_dataset = GDALOpenEx(filename.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
    if (m_dataset == nullptr) {
        throw std::runtime_error("Failed to open " + filename);
    }

    bool numeric = std::all_of(layer.begin(), layer.end(), [](char c) { return std::isdigit(c); });

    if (numeric) {
        m_layer = GDALDatasetGetLayer(m_dataset, std::stoi(layer));
    } else {
        m_layer = GDALDatasetGetLayerByName(m_dataset, layer.c_str());
    }

    if (m_layer == nullptr) {
        throw std::runtime_error("No layer " + layer + " found in " + filename);
    }

    OGR_L_ResetReading(m_layer);
}

GDALDatasetWrapper::GDALDatasetWrapper(GDALDatasetWrapper&& other) noexcept
  : m_dataset(other.m_dataset)
  , m_layer(other.m_layer)
  , m_feature(std::move(other.m_feature))
  , m_layer_is_sql(other.m_layer_is_sql)
{
    other.m_dataset = nullptr;
    other.m_layer = nullptr;
    other.m_layer_is_sql = false;
}

GDALDatasetWrapper&
GDALDatasetWrapper::operator=(GDALDatasetWrapper&& other) noexcept
{
    m_dataset = other.m_dataset;
    m_layer = other.m_layer;
    m_feature = std::move(other.m_feature);
    m_layer_is_sql = other.m_layer_is_sql;

    other.m_dataset = nullptr;
    other.m_layer = nullptr;
    other.m_layer_is_sql = false;

    return *this;
}

void
GDALDatasetWrapper::set_select(const std::vector<std::string>& cols)
{
    const char* layer_name = OGR_L_GetName(m_layer);
    std::stringstream sql;

    sql << "SELECT ";
    for (std::size_t i = 0; i < cols.size(); i++) {
        if (i > 0) {
            sql << ", ";
        }
        sql << cols[i];
    }
    sql << " FROM " << layer_name;

    std::string sql_str = sql.str();

    m_layer = GDALDatasetExecuteSQL(m_dataset, sql.str().c_str(), nullptr, "OGRSQL");
    m_layer_is_sql = true;
}

bool
GDALDatasetWrapper::next()
{
    OGRFeatureH raw_feature = OGR_L_GetNextFeature(m_layer);
    m_feature = GDALFeature(raw_feature);
    return raw_feature != nullptr;
}

void
GDALDatasetWrapper::copy_field(const std::string& name, OGRLayerH copy_to) const
{
    auto src_layer_defn = OGR_L_GetLayerDefn(m_layer);
    auto src_index = OGR_FD_GetFieldIndex(src_layer_defn, name.c_str());

    if (src_index == -1) {
        throw std::runtime_error("Cannot find field " + name);
    }

    auto src_field_defn = OGR_FD_GetFieldDefn(src_layer_defn, src_index);

    OGR_L_CreateField(copy_to, src_field_defn, true);
}

GDALDatasetWrapper::~GDALDatasetWrapper()
{
    if (m_layer_is_sql) {
        GDALDatasetReleaseResultSet(m_dataset, m_layer);
    }
    GDALClose(m_dataset);
}
}
