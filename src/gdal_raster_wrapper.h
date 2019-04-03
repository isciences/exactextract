// Copyright (c) 2018-2019 ISciences, LLC.
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

#ifndef EXACTEXTRACT_GDAL_RASTER_WRAPPER_H
#define EXACTEXTRACT_GDAL_RASTER_WRAPPER_H

#include "box.h"
#include "grid.h"
#include "raster.h"

namespace exactextract {

    class GDALRasterWrapper {

    public:
        GDALRasterWrapper(const std::string &filename, int bandnum);

        const Grid<bounded_extent> &grid() const {
            return m_grid;
        }

        void set_name(const std::string & name) {
            m_name = name;
        }

        std::string name() const {
            return m_name;
        }

        Raster<double> read_box(const Box &box);

        ~GDALRasterWrapper();

        GDALRasterWrapper(const GDALRasterWrapper &) = delete;
        GDALRasterWrapper(GDALRasterWrapper &&) noexcept;
    private:
        using GDALDatasetH=void*;
        using GDALRasterBandH=void*;

        GDALDatasetH m_rast;
        GDALRasterBandH m_band;
        double m_nodata_value;
        bool m_has_nodata;
        Grid<bounded_extent> m_grid;
        std::string m_name;

        void compute_raster_grid();
    };
}

#endif //EXACTEXTRACT_GDAL_RASTER_WRAPPER_H
