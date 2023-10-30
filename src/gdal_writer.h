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

#ifndef EXACTEXTRACT_GDAL_WRITER_H
#define EXACTEXTRACT_GDAL_WRITER_H

#include "output_writer.h"

namespace exactextract {

    class GDALDatasetWrapper;

    class GDALWriter : public OutputWriter {

    public:
        explicit GDALWriter(const std::string & filename);

        ~GDALWriter() override;

        static std::string get_driver_name(const std::string & filename);

        void add_operation(const Operation & op) override;

        std::unique_ptr<Feature> create_feature() override;

        void write(const Feature& f) override;

        void add_id_field(const std::string & field_name, const std::string & field_type);

        void copy_field(const GDALDatasetWrapper& w, const std::string& field_name);

        void copy_id_field(const GDALDatasetWrapper & w);

    protected:
        using GDALDatasetH = void*;
        using OGRLayerH = void*;

        GDALDatasetH m_dataset;
        OGRLayerH m_layer;
        bool id_field_defined = false;
    };

}

#endif //EXACTEXTRACT_GDAL_WRITER_H
