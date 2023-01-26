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
 * @file gdal_writer.h
 * @version 0.1
 * @date 2022-03-24
 * 
 * Changelog:
 *  version 0.1
 *      Nels Frazier (nfrazier@lynker.com) implemented write_coverage
 *      Nels Frazier (nfrazier@lynker.com) marked add_id_field, copy_id_field as virtual
 *      Nels Frazier (nfrazier@lynker.com) made OGRLayerH protected so it can be used by subclasses
 * 
 */
#ifndef EXACTEXTRACT_GDAL_WRITER_H
#define EXACTEXTRACT_GDAL_WRITER_H

#include "raster.h"
#include "output_writer.h"

namespace exactextract {

    class GDALDatasetWrapper;

    class GDALWriter : public OutputWriter {

    public:
        explicit GDALWriter(const std::string & filename);

        ~GDALWriter() override;

        static std::string get_driver_name(const std::string & filename);

        void add_operation(const Operation & op) override;

        void set_registry(const StatsRegistry* reg) override;

        void write(const std::string & fid) override;

        void write_coverage(const std::string & fid, const Raster<float> & raster, const Grid<bounded_extent> & common_grid) override {
            (void)fid;
            (void)raster;
            (void)common_grid;
            throw std::runtime_error("Unimplemented Error: GDALWriter doesn't implement write_coverage, use 'write' instead");
        };

        virtual void add_id_field(const std::string & field_name, const std::string & field_type);

        virtual void copy_id_field(const GDALDatasetWrapper & w);

    protected:
        using OGRLayerH = void*;
        OGRLayerH m_layer;

    private:
        using GDALDatasetH = void*;

        GDALDatasetH m_dataset;
        const StatsRegistry* m_reg;
        bool id_field_defined = false;
    };

}

#endif //EXACTEXTRACT_GDAL_WRITER_H
