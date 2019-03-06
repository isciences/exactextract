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

#ifndef EXACTEXTRACT_NETCDF_WRITER_H
#define EXACTEXTRACT_NETCDF_WRITER_H

#include "output_writer.h"

#include <netcdf>
#include <numeric>
#include <vector>

namespace exactextract {

    class BaseVariable;

class NetCDFWriter : public OutputWriter {
public:
    NetCDFWriter(std::string filename, const std::string & id_field_name) :
    m_ncout{filename, netCDF::NcFile::replace, netCDF::NcFile::nc4classic}
    {}

    void write(const std::string & fid) override;

    void add_operation(const Operation & op) override;

    void set_registry(const StatsRegistry* reg) override {
        m_reg = reg;
    }

    void finish() override;

private:
    netCDF::NcFile m_ncout;
    std::vector<const Operation*> m_ops;
    const StatsRegistry* m_reg;
    bool m_initialized = false;

#if 0
    union Datum {
        double dbl;
        float flt;
        int32_t int32;
        int64_t int64;
    };

    struct Variable {
        std::string type;
        std::string name;
        std::vector<Datum> data;
    };
#endif

    class Variable {
    public:
        Variable(std::string name, std::string type) :
            m_name{std::move(name)},
            m_type{std::move(type)},
            m_data{std::make_unique<std::vector<uint8_t>>(16)}
        {}

        template<typename T>
        void put(T d) {
            if (m_pos + sizeof(T) >= m_data->size()) {
                m_data->resize(2 * m_data->size());
            }

            std::cout << "Writing " << sizeof(T) << " at " << m_pos << std::endl;
            std::cout << "Size is " << m_data->size() << std::endl;

            auto loc = reinterpret_cast<T*>(m_data->data() + m_pos);
            *loc = d;
            m_pos += sizeof(T);
        }

        void write(netCDF::NcVar & v) {
            auto type = v.getType();
            auto dims = v.getDims();
            auto expected_size = std::accumulate(dims.begin(),
                    dims.end(),
                    0ul,
                    [](const auto & acc, const auto & elem) {
                        return std::max(acc, 1ul) * elem.getSize();
                    }
            );

            if (type == netCDF::ncDouble) {
                check_size<double>(expected_size);
                v.putVar(as<double>());
            } else if (type == netCDF::ncFloat) {
                check_size<float>(expected_size);
                v.putVar(as<float>());
            } else if (type == netCDF::ncInt) {
                check_size<int32_t>(expected_size);
                v.putVar(as<int32_t>());
            } else if (type == netCDF::ncInt64) {
                check_size<int64_t>(expected_size);
                v.putVar(as<int64_t>());
            } else if (type == netCDF::ncShort) {
                check_size<int16_t>(expected_size);
                v.putVar(as<int16_t>());
            } else if (type == netCDF::ncByte) {
                check_size<int8_t>(expected_size);
                v.putVar(as<int8_t>());
            } else {
                throw std::runtime_error("Unhandled variable type.");
            }
        }

        const std::string& type() const {
            return m_type;
        }

        const std::string& name() const {
            return m_name;
        }

    private:
        template<typename T>
        T* as() {
            return reinterpret_cast<T*>(m_data.get());
        }

        template<typename T>
        void check_size(size_t expected) {
            if (m_pos != expected*sizeof(T)) {
                throw std::runtime_error("Unexpected size when writing to variable.");
            }
        }

        size_t m_pos = 0;
        std::unique_ptr<std::vector<uint8_t>> m_data;
        std::string m_name;
        std::string m_type;
    };

    std::vector<Variable> m_data;
};
}


#endif //EXACTEXTRACT_NETCDF_WRITER_H
