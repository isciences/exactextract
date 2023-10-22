// Copyright (c) 2023 ISciences, LLC.
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

#include "operation.h"

namespace exactextract {

class CoverageOperation : public Operation
{

  public:
    enum class AreaMethod
    {
        NONE,
        CARTESIAN,
        SPHERICAL
    };

    struct Options
    {
        // bool coverage_areas = false;   // whether to output coverage_area instead of converage_fraction
        bool include_xy = false;          // whether to include columns for cell center coordinates
        bool include_cell = false;        // whether to include a (0-based) cell index
        bool include_values = false;      // whether to include values of the value raster
        bool include_weights = false;     // whether to include values of the weighting raster
        AreaMethod area_method = AreaMethod::NONE; // optional method by which to calculate a cell area column
    };

    CoverageOperation(const std::string& p_name,
                      RasterSource* p_values,
                      RasterSource* p_weights,
                      Options options)
      : Operation("coverage", p_name, p_values, p_weights)
      , m_coverage_opts(options)
    {
        m_field_names.clear();

        if (m_coverage_opts.include_cell) {
            m_field_names.push_back("cell");
        }

        if (m_coverage_opts.include_xy) {
            m_field_names.push_back("x");
            m_field_names.push_back("y");
        }

        if (m_coverage_opts.area_method != AreaMethod::NONE) {
            m_field_names.push_back("area");
        }

        m_field_names.push_back("coverage_fraction");

        if (m_coverage_opts.include_values) {
            m_field_names.push_back(values->name());
        }

        if (m_coverage_opts.include_weights && weights != nullptr) {
            m_field_names.push_back(weights->name());
        }
    }

    /// Method which which a CoverateProcessor can save a value to be applied
    /// the next time set_result is called. A bit of a kludge.
    void save_coverage(const CoverageValue<double, double>& last_coverage)
    {
        m_last_coverage = last_coverage;
    }

    virtual void set_result(const StatsRegistry& reg, const std::string& fid, Feature& f_out) const
    {
        (void)reg;
        (void)fid;

        const auto& loc = m_last_coverage;

        f_out.set("coverage_fraction", loc.coverage);

        if (m_coverage_opts.include_cell) {
            f_out.set("cell", loc.cell);
        }

        if (m_coverage_opts.include_xy) {
            f_out.set("x", loc.x);
            f_out.set("y", loc.y);
        }

        if (m_coverage_opts.area_method != AreaMethod::NONE) {
            f_out.set("area", loc.area);
        }

        if (m_coverage_opts.include_values) {
            f_out.set(values->name(), loc.value);
        }

        if (m_coverage_opts.include_weights && weights != nullptr) {
            f_out.set(weights->name(), loc.value);
        }
    }

    const Options& options() const
    {
        return m_coverage_opts;
    }

  private:
    Options m_coverage_opts;
    CoverageValue<double, double> m_last_coverage;
};

}
