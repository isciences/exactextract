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
 * @file coverage_writer.cpp
 * @author Nels Frazier (nfrazier@lynker.com)
 * @brief Custom output writer for writing coverage fraction and cell number
 * @version 0.1
 * @date 2022-03-24
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "coverage_writer.h"
#include "gdal_dataset_wrapper.h"
#include "stats_registry.h"
#include "utils.h"

#include "gdal.h"
#include "ogr_api.h"
#include "cpl_string.h"

#include <stdexcept>

namespace exactextract {
    //Static initialization
    const std::string CoverageWriter::m_coverage_field = "coverage_fraction";
    const std::string CoverageWriter::m_cell_number_field = "cell_number";

    CoverageWriter::CoverageWriter(const std::string & filename):
        GDALWriter(filename), fields_initialized(false)
    {
        
    }

    void CoverageWriter::initialize_fields()
    {
        if(!fields_initialized)
        {
            ///Make sure coverage_fraction and cell_number fields are created
            auto cov_def = OGR_Fld_Create(m_coverage_field.c_str(), OFTReal);
            OGR_L_CreateField(m_layer, cov_def, true);
            OGR_Fld_Destroy(cov_def);
            auto cell_def = OGR_Fld_Create(m_cell_number_field.c_str(), OFTInteger64);
            OGR_L_CreateField(m_layer, cell_def, true);
            OGR_Fld_Destroy(cell_def);

            ///Make sure coverage_fraction and cell_number fields are created
            auto row_def = OGR_Fld_Create("row", OFTInteger64);
            OGR_L_CreateField(m_layer, row_def, true);
            OGR_Fld_Destroy(row_def);
            auto col_def = OGR_Fld_Create("col", OFTInteger64);
            OGR_L_CreateField(m_layer, col_def, true);
            OGR_Fld_Destroy(col_def);

            fields_initialized = true;
        }
    }

    void CoverageWriter::add_id_field(const std::string & field_name, const std::string & field_type){
        m_id_field = field_name;
        GDALWriter::add_id_field(field_name, field_type);
    }

    void CoverageWriter::copy_id_field(const GDALDatasetWrapper & w){
        m_id_field = w.id_field();
        GDALWriter::copy_id_field(w);
    }

    void CoverageWriter::write(const std::string & fid) {
        //Since this writer is not intended to write per feature stat data,
        //but is instead intended to write raster coverage
        //we intentionally override the `write` function and throw an error
        //if it is used.  use `write_coverage` instead

        (void)fid; //suppress unused parameter compiler warning/error
        throw std::runtime_error("Unimplemented Error: CoverageWriter doesn't implement write, use 'write_coverage' instead");
    }

    void CoverageWriter::write_coverage(const std::string & fid, const Raster<float>& raster, const Grid<bounded_extent> & common_grid) {
        initialize_fields();
        for (size_t i = 0; i < raster.rows(); i++) {
                for (size_t j = 0; j < raster.cols(); j++) {
                    float pct_cov = raster(i, j);
                    
                    if (pct_cov > 0) {
                        size_t cell_number = raster.grid().map_to_grid_cell(i, j, common_grid);
                        //Create the feature
                        auto feature = OGR_F_Create(OGR_L_GetLayerDefn(m_layer));
                        const auto id_field_pos = OGR_F_GetFieldIndex(feature, m_id_field.c_str());
                        OGR_F_SetFieldString(feature, id_field_pos, fid.c_str());
                        const auto cov_field_pos = OGR_F_GetFieldIndex(feature, m_coverage_field.c_str());
                        const auto cell_field_pos = OGR_F_GetFieldIndex(feature, m_cell_number_field.c_str());
                        const auto row_field_pos = OGR_F_GetFieldIndex(feature, "row");
                        const auto col_field_pos = OGR_F_GetFieldIndex(feature, "col");
                        OGR_F_SetFieldDouble(feature, cov_field_pos, pct_cov);
                        OGR_F_SetFieldDouble(feature, row_field_pos, i+common_grid.row_offset(raster.grid()));
                        OGR_F_SetFieldDouble(feature, col_field_pos, j+common_grid.col_offset(raster.grid()));
                        OGR_F_SetFieldDouble(feature, cell_field_pos, cell_number);
                        if (OGR_L_CreateFeature(m_layer, feature) != OGRERR_NONE) {
                            throw std::runtime_error("Error writing results for record: " + fid);
                        }
                        OGR_F_Destroy(feature);
                    }
                }
            } 
    }

}
