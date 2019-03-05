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

#ifndef EXACTEXTRACT_PROCESSOR_H
#define EXACTEXTRACT_PROCESSOR_H

#include <iostream>
#include <string>

#include "gdal_dataset_wrapper.h"
#include "output_writer.h"
#include "stats_registry.h"


namespace exactextract {
    class Processor {

    public:
        Processor(GDALDatasetWrapper & ds, OutputWriter & out, const std::string & id_field) :
                m_shp{ds},
                m_reg{},
                m_output{out},
                m_field_name{id_field}
        {
            m_geos_context = initGEOS_r(nullptr, nullptr);
            m_output.set_registry(&m_reg);
        }

        virtual ~Processor() {
            finishGEOS_r(m_geos_context);
        }

        void add_operation(const Operation & op) {
            m_operations.push_back(op);
        }

        virtual void process()= 0;

    protected:

        template<typename T>
        void progress(const T & name) {
            std::cout << std::endl << "Processing " << name << std::flush;
        }

        void progress() {
            std::cout << "." << std::flush;
        }

        GEOSContextHandle_t m_geos_context;
        StatsRegistry m_reg;

        OutputWriter& m_output;

        GDALDatasetWrapper& m_shp;

        std::string m_field_name;

        bool store_values=false;

        std::vector<Operation> m_operations;

        size_t max_cells_in_memory = 1000000L;
    };
}

#endif //EXACTEXTRACT_PROCESSOR_H
