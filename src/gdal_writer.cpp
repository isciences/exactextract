// Copyright (c) 2019 ISciences, LLC.
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
#include "gdal_dataset_wrapper.h"
#include "stats_registry.h"
#include "utils.h"

#include "gdal.h"
#include "ogr_api.h"
#include "cpl_string.h"

#include <stdexcept>

namespace exactextract {
    GDALWriter::GDALWriter(const std::string & filename, const std::string & driver_name)
    {
        if (driver_name.empty()) {
            driver_name = get_driver_name(filename);
        }
        auto driver = GDALGetDriverByName(driver_name.c_str());

        if (driver == nullptr) {
            throw std::runtime_error("Could not load output driver: " + driver_name);
        }

        char** layer_creation_options = nullptr;
        if (driver_name == "NetCDF") {
            //creation_options = CSLSetNameValue(creation_options, "FORMAT", "NC3"); // crashes w/NC4C
            layer_creation_options = CSLSetNameValue(layer_creation_options, "RECORD_DIM_NAME", "id");
        }

        m_dataset = GDALCreate(driver, filename.c_str(), 0, 0, 0, GDT_Unknown, nullptr);
        m_layer = GDALDatasetCreateLayer(m_dataset, "output", nullptr, wkbNone, layer_creation_options);

        CSLDestroy(layer_creation_options);
    }

    GDALWriter::~GDALWriter() {
        if (m_dataset != nullptr) {
            GDALClose(m_dataset);
        }
    }

    void GDALWriter::copy_id_field(const GDALDatasetWrapper & w) {
        if (id_field_defined) {
            throw std::runtime_error("ID field already defined.");
        }

        w.copy_field(w.id_field(), m_layer);
        id_field_defined = true;
    }

    void GDALWriter::add_id_field(const std::string & field_name, const std::string & field_type) {
        if (id_field_defined) {
            throw std::runtime_error("ID field already defined.");
        }

        OGRFieldType ogr_type;
        if (field_type == "int" || field_type == "int32")  {
            ogr_type = OFTInteger;
        } else if (field_type == "long" || field_type == "int64") {
            ogr_type = OFTInteger64;
        } else if (field_type == "text" || field_type == "string") {
            ogr_type = OFTString;
        } else if (field_type == "double" || field_type == "float" || field_type == "real") {
            ogr_type = OFTReal;
        } else {
            throw std::runtime_error("Unknown field type: " + field_type);
        }

        auto def = OGR_Fld_Create(field_name.c_str(), ogr_type);
        OGR_L_CreateField(m_layer, def, true);
        OGR_Fld_Destroy(def);
        id_field_defined = true;
    }

    void GDALWriter::add_operation(const Operation & op) {
        if (!id_field_defined) {
            throw std::runtime_error("Must define ID field before adding operations.");
        }

        // TODO set type here?
        auto def = OGR_Fld_Create(op.name.c_str(), OFTReal);
        OGR_L_CreateField(m_layer, def, true);
        OGR_Fld_Destroy(def);

        m_ops.push_back(&op);
    }

    void GDALWriter::set_registry(const StatsRegistry* reg) {
        m_reg = reg;
    }

    void GDALWriter::write(const std::string & fid) {
        auto feature = OGR_F_Create(OGR_L_GetLayerDefn(m_layer));

        OGR_F_SetFieldString(feature, 0, fid.c_str());

        for (const auto &op : m_ops) {
            if (m_reg->contains(fid, *op)) {
                const auto field_pos = OGR_F_GetFieldIndex(feature, op->name.c_str());
                const auto &stats = m_reg->stats(fid, *op);

                // TODO store between features
                auto fetcher = op->result_fetcher();

                auto val = fetcher(stats);
                if (val.has_value()) {
                    OGR_F_SetFieldDouble(feature, field_pos, val.value());
                } else {
                    OGR_F_SetFieldDouble(feature, field_pos, std::numeric_limits<double>::quiet_NaN());
                }
            }
        }

        if (OGR_L_CreateFeature(m_layer, feature) != OGRERR_NONE) {
            throw std::runtime_error("Error writing results for record: " + fid);
        }
        OGR_F_Destroy(feature);
    }

    std::string GDALWriter::get_driver_name(const std::string & filename) {
        if (ends_with(filename, ".csv")) {
            return "CSV";
        } else if (ends_with(filename, ".dbf")) {
            return "ESRI Shapefile";
        } else if (ends_with(filename, ".nc")) {
            return "NetCDF";
        } else if (starts_with(filename, "PG:")) {
            return "PostgreSQL";
        } else {
            throw std::runtime_error("Unknown output format: " + filename);
        }
    }

}
