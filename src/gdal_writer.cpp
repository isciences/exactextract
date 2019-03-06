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
#include "stats_registry.h"
#include "utils.h"

#include "gdal.h"
#include "ogr_api.h"

namespace exactextract {
    GDALWriter::GDALWriter(const std::string & filename, const std::string & id_field)
    {
        auto driver_name = get_driver_name(filename);
        auto driver = GDALGetDriverByName(driver_name.c_str());

        if (driver == nullptr) {
            throw std::runtime_error("Could not load output driver: " + driver_name);
        }

        m_dataset = GDALCreate(driver, filename.c_str(), 0, 0, 0, GDT_Unknown, nullptr);
        m_layer = GDALDatasetCreateLayer(m_dataset, "output", nullptr, wkbNone, nullptr);

        auto id_def = OGR_Fld_Create(id_field.c_str(), OFTString);
        OGR_Fld_SetWidth(id_def, 32); // FIXME
        OGR_L_CreateField(m_layer, id_def, true);
        OGR_Fld_Destroy(id_def);
    }

    GDALWriter::~GDALWriter() {
        if (m_dataset != nullptr) {
            GDALClose(m_dataset);
        }
    }

    void GDALWriter::add_operation(const Operation & op) {
        auto field_name = varname(op);

        // TODO set type here?
        auto def = OGR_Fld_Create(field_name.c_str(), OFTReal);
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
                const auto field_pos = OGR_F_GetFieldIndex(feature, varname(*op).c_str());
                const auto &stats = m_reg->stats(fid, *op);

                if (op->stat == "mean") {
                    OGR_F_SetFieldDouble(feature, field_pos, stats.mean());
                } else if (op->stat == "sum") {
                    OGR_F_SetFieldDouble(feature, field_pos, stats.sum());
                } else {
                    // FIXME support all stats
                    OGR_F_Destroy(feature);
                    throw std::runtime_error("Not implemented.");
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
