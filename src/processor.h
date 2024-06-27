// Copyright (c) 2019-2024 ISciences, LLC.
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

#pragma once

#include <cstdarg>
#include <iostream>
#include <string>

#include "feature_source.h"
#include "operation.h"
#include "output_writer.h"
#include "stats_registry.h"

static void
errorHandler(const char* fmt, ...)
{

    char buf[BUFSIZ], *p;
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    p = buf + strlen(buf) - 1;
    if (strlen(buf) > 0 && *p == '\n')
        *p = '\0';

    std::cerr << buf << std::endl;
}

namespace exactextract {
/**
 * @brief The Processor class applies one or more operations to all features in the input dataset,
 *        writing the results to an OutputWriter. Subclasses define the manner in which this happens
 *        by overriding the `process` method.
 */
class Processor
{

  public:
    Processor(FeatureSource& ds, OutputWriter& out)
      : m_geos_context{ initGEOS_r(errorHandler, errorHandler) }
      , m_output{ out }
      , m_shp{ ds }
    {
    }

    virtual ~Processor()
    {
        finishGEOS_r(m_geos_context);
    }

    virtual void process() = 0;

    void add_operation(const Operation& op)
    {
        m_operations.push_back(op.clone());
        m_reg.prepare(op);
        m_output.add_operation(op);
    }

    void include_col(const std::string& col)
    {
        m_include_cols.push_back(col);

        m_output.add_column(col);
    }

    void include_geometry()
    {
        m_include_geometry = true;

        m_output.add_geometry();
    }

    void set_max_cells_in_memory(size_t n)
    {
        m_max_cells_in_memory = n;
    }

    void show_progress(bool val)
    {
        m_show_progress = val;
    }

    void set_progress_fn(std::function<void(std::string_view)> fn)
    {
        m_progress_fn = fn;
    }

    void write_result(const Feature& f_in)
    {
        auto f_out = m_output.create_feature();
        if (m_include_geometry) {
            f_out->set_geometry(f_in.geometry());
        }
        for (const auto& col : m_include_cols) {
            f_out->set(col, f_in);
        }
        for (const auto& op : m_operations) {
            op->set_result(m_reg, f_in, *f_out);
        }
        m_output.write(*f_out);
        m_reg.flush_feature(f_in);
    }

  protected:
    void progress(std::string_view message) const
    {
        if (!m_show_progress) {
            return;
        }

        if (m_progress_fn) {
            (m_progress_fn)(message);
            return;
        }

        std::cout << "." << std::flush;
    }

    StatsRegistry m_reg;

    GEOSContextHandle_t m_geos_context;

    OutputWriter& m_output;

    FeatureSource& m_shp;

    bool m_show_progress = false;
    bool m_include_geometry = false;

    std::vector<std::unique_ptr<Operation>> m_operations;

    std::vector<std::string> m_include_cols;

    size_t m_max_cells_in_memory = 1000000L;

    std::function<void(std::string_view)> m_progress_fn;
};
}
