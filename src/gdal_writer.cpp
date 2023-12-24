// Copyright (c) 2019-2023 ISciences, LLC.
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

#include "gdal_writer.h"
#include "coverage_operation.h"
#include "gdal_dataset_wrapper.h"
#include "gdal_feature.h"
#include "operation.h"
#include "stats_registry.h"
#include "utils.h"

#include "cpl_string.h"
#include "gdal.h"
#include "ogr_api.h"

#include <stdexcept>

namespace exactextract {
GDALWriter::GDALWriter(const std::string& filename)
{
    auto driver_name = get_driver_name(filename);
    auto driver = OGRGetDriverByName(driver_name.c_str());

    if (driver == nullptr) {
        throw std::runtime_error("Could not load output driver: " + driver_name);
    }

    char** layer_creation_options = nullptr;
    if (driver_name == "NetCDF") {
        // creation_options = CSLSetNameValue(creation_options, "FORMAT", "NC3"); // crashes w/NC4C
        layer_creation_options = CSLSetNameValue(layer_creation_options, "RECORD_DIM_NAME", "id");
    }

    m_dataset = GDALCreate(driver, filename.c_str(), 0, 0, 0, GDT_Unknown, nullptr);
    m_layer = GDALDatasetCreateLayer(m_dataset, "output", nullptr, wkbNone, layer_creation_options);

    CSLDestroy(layer_creation_options);
}

GDALWriter::~GDALWriter()
{
    if (m_dataset != nullptr) {
        GDALClose(m_dataset);
    }
}

std::unique_ptr<Feature>
GDALWriter::create_feature()
{
    auto defn = OGR_L_GetLayerDefn(m_layer);
    return std::make_unique<GDALFeature>(OGR_F_Create(defn));
}

void
GDALWriter::copy_field(const GDALDatasetWrapper& w, const std::string& name)
{
    w.copy_field(name, m_layer);
}

void
GDALWriter::add_operation(const Operation& op)
{
    for (const auto& field_name : op.field_names()) {
        // TODO set type here
        auto def = OGR_Fld_Create(field_name.c_str(), OFTReal);
        OGR_L_CreateField(m_layer, def, true);
        OGR_Fld_Destroy(def);
    }
}

void
GDALWriter::write(const Feature& f)
{
    OGRErr err = OGRERR_NONE;

    if (const GDALFeature* gf = dynamic_cast<const GDALFeature*>(&f)) {
        err = OGR_L_CreateFeature(m_layer, gf->raw());
    } else {
        throw std::runtime_error("not supported yet");
    }

    if (err != OGRERR_NONE) {
        throw std::runtime_error("Error writing results.");
    }
}

std::string
GDALWriter::get_driver_name(const std::string& filename)
{
    if (ends_with(filename, ".csv")) {
        return "CSV";
    } else if (ends_with(filename, ".dbf")) {
        return "ESRI Shapefile";
    } else if (ends_with(filename, ".nc")) {
        return "NetCDF";
    } else if (starts_with(filename, "PG:")) {
        return "PostgreSQL";
    } else if (ends_with(filename, ".parquet")) {
        return "Parquet";
    } else {
        throw std::runtime_error("Unknown output format: " + filename);
    }
}

}
