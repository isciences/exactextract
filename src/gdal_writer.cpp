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

/**
 * @file gdal_writer.cpp
 * @version 0.1
 * @date 2022-03-24
 * 
 * Changelog:
 *  version 0.1
 *      Nels Frazier (nfrazier@lynker.com) refactor write to handle vector results
 *      Nels Frazier (nfrazier@lynker.com) add geojson output driver
 * 
 */

#include "gdal_writer.h"
#include "gdal_dataset_wrapper.h"
#include "stats_registry.h"
#include "utils.h"

#include "gdal.h"
#include "ogr_api.h"
#include "cpl_string.h"

#include <stdexcept>

namespace exactextract {
    GDALWriter::GDALWriter(const std::string & filename)
    {
        auto driver_name = get_driver_name(filename);
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
        //container of features
        using customMap = std::vector<OGRFeatureH>;
        customMap feature_map;
        OGRFeatureH feature_h;
        for (const auto &op : m_ops) {
            //An aggregate and/or first cell
            if( feature_map.size() == 0 ){
                //if no features processed yet, create a feature
                feature_h = OGR_F_Create(OGR_L_GetLayerDefn(m_layer));
                OGR_F_SetFieldString(feature_h, 0, fid.c_str());
                feature_map.push_back(feature_h);
            }

            if (m_reg->contains(fid, *op)) {
                //reuse the info from first feature
                const auto field_pos = OGR_F_GetFieldIndex(feature_map[0], op->name.c_str());
                const auto &stats = m_reg->stats(fid, *op);
                try{
                    // TODO store between features
                    auto fetcher = op->result_fetcher();
                    auto val = fetcher(stats);
                    if (val.has_value()) {
                        OGR_F_SetFieldDouble(feature_map[0], field_pos, val.value());
                    } else {
                        OGR_F_SetFieldDouble(feature_map[0], field_pos, std::numeric_limits<double>::quiet_NaN());
                    }
                } catch(std::runtime_error& e)
                {   //Handle fector results
                    auto fetcher = op->vector_result_fetcher();
                    auto val = fetcher(stats);

                    if (val.has_value() && val.value().size() > 0) {
                        auto vec = val.value();
                        //Write the first feature using the existing feature
                         OGR_F_SetFieldDouble(feature_map[0], field_pos, vec[0]);

                        for(size_t i = 1; i < vec.size(); i++){
                            //create a new feature with the same id for each other value
                            if(feature_map.size() == i){
                                auto inner_feature = OGR_F_Create(OGR_L_GetLayerDefn(m_layer));
                                OGR_F_SetFieldString(inner_feature, 0, fid.c_str());
                                feature_map.push_back(inner_feature);
                            }
                             OGR_F_SetFieldDouble(feature_map[i], field_pos, vec[i]);
                        }

                    } else {
                        OGR_F_SetFieldDouble(feature_map[0], field_pos, std::numeric_limits<double>::quiet_NaN());
                    }
                }
                
            }
        }

        for(const auto& handle : feature_map){
            if (OGR_L_CreateFeature(m_layer, handle) != OGRERR_NONE) {
            throw std::runtime_error("Error writing results for record: " + fid);
            }
            OGR_F_Destroy(handle);
        }
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
        } else if (ends_with(filename, "json")){
            return "GeoJSON";
        }else {
            throw std::runtime_error("Unknown output format: " + filename);
        }
    }

}
