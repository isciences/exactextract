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

#ifndef EXACTEXTRACT_COVERAGE_WRITER_H
#define EXACTEXTRACT_COVERAGE_WRITER_H
/**
 * @file coverage_writer.h
 * @author Nels Frazier (nfrazier@lynker.com)
 * @brief Custom output writer for writing coverage fraction and cell number
 * @version 0.1
 * @date 2022-03-24
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "raster.h"
#include "gdal_writer.h"

namespace exactextract {

    /*
        Custom writer for writing coverage fraction
    */
    class CoverageWriter: public GDALWriter {

    public:
        explicit CoverageWriter(const std::string & filename);

        ~CoverageWriter(){};

        void write(const std::string & fid) override;

        void add_id_field(const std::string & field_name, const std::string & field_type) override;

        void copy_id_field(const GDALDatasetWrapper & w) override;

        void write_coverage(const std::string & fid, const Raster<float> & raster, const Grid<bounded_extent> & common_grid) override;

    private:
        void inline initialize_fields();
        static const std::string m_coverage_field;
        static const std::string m_cell_number_field;
        std::string m_id_field;
        bool fields_initialized;
    };

}

#endif //EXACTEXTRACT_COVERAGE_WRITER_H
