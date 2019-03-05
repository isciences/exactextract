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

#ifndef EXACTEXTRACT_CSV_WRITER_H
#define EXACTEXTRACT_CSV_WRITER_H

#include "output_writer.h"
#include "stats_registry.h"

#include <fstream>

namespace exactextract {

    class CSVWriter : public OutputWriter {
    public:

        CSVWriter(const std::string & filename, const std::string & id_field_name) :
            m_field_name{id_field_name}
        {
            m_csvout.open(filename);
        }

        virtual void add_operation(const Operation & op) override {
            m_ops.push_back(&op);
        }

        virtual void set_registry(const StatsRegistry* reg) override {
            m_reg = reg;
        }

        void write(const std::string & fid) override {
            if (!m_initialized) {
                write_header();
                m_initialized = true;
            }

            m_csvout << fid;

            for (const auto & op : m_ops) {
                m_csvout << m_sepchar;

                if (m_reg->contains(fid, *op)) {
                    const auto& stats = m_reg->stats(fid, *op);

                    if (op->stat == "mean") {
                        m_csvout << stats.mean();
                    } else if (op->stat == "sum") {
                        m_csvout << stats.sum();
                    } else {
                        // FIXME
                    }
                }
            }

            m_csvout << "\n";
        }

    private:
        void write_header() {
            m_csvout << m_field_name;

            for (const auto & op : m_ops) {
                m_csvout << m_sepchar << varname(*op);
            }

            m_csvout << "\n";
        }

        char m_sepchar = ',';
        bool m_initialized = false;

        std::ofstream m_csvout;

        std::string m_field_name;

        const StatsRegistry* m_reg;
    };


}

#endif //EXACTEXTRACT_CSV_WRITER_H
