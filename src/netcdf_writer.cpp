#include <memory>

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

#include "netcdf_writer.h"

#include "stats_registry.h"

namespace exactextract {

    void NetCDFWriter::add_operation(const Operation & op) {
        m_ops.push_back(&op);
        m_data.emplace_back(varname(op), "double", 100);
    }

    void NetCDFWriter::write(const std::string & fid) {

        for (size_t i = 0; i < m_ops.size(); i++) {
            const auto& op = m_ops[i];

            if (m_reg->contains(fid, *op)) {
                const auto &stats = m_reg->stats(fid, *op);

                if (op->stat == "mean") {
                    m_data[i].put(stats.mean());
                } else if (op->stat == "sum") {
                    m_data[i].put(stats.sum());
                } else {
                    // FIXME
                    throw std::runtime_error("Not implemented.");
                }
            }
        }
    }

    void NetCDFWriter::finish() {
        using namespace netCDF;

        NcDim id = m_ncout.addDim("id", m_data.size());

        std::vector<NcVar> vars;
        vars.reserve(m_data.size());

        for (auto &i : m_data) {
            vars.push_back(m_ncout.addVar(i.name(), i.type(), "id"));
        }

        for (size_t i = 0; i < vars.size(); i++) {
            m_data[i].write(vars[i]);
        }
    }

}
