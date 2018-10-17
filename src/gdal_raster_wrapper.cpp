// Copyright (c) 2018 ISciences, LLC.
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

#include "gdal_raster_wrapper.h"

namespace exactextract {

    Raster<double> GDALRasterWrapper::read_box(const Box &box) {
        auto cropped_grid = m_grid.shrink_to_fit(box);
        Raster<double> vals(cropped_grid);
        if (m_has_nodata) {
            vals.set_nodata(m_nodata_value);
        }

        auto error = GDALRasterIO(m_band,
                                  GF_Read,
                                  (int) cropped_grid.col_offset(m_grid),
                                  (int) cropped_grid.row_offset(m_grid),
                                  (int) cropped_grid.cols(),
                                  (int) cropped_grid.rows(),
                                  vals.data().data(),
                                  (int) cropped_grid.cols(),
                                  (int) cropped_grid.rows(),
                                  GDT_Float64,
                                  0,
                                  0);

        if (error) {
            throw std::runtime_error("Error reading from raster.");
        }

        return vals;
    }

    Grid<bounded_extent> GDALRasterWrapper::compute_raster_grid() {
        double adfGeoTransform[6];
        if (GDALGetGeoTransform(m_rast, adfGeoTransform) != CE_None) {
            throw std::runtime_error("Error reading transform");
        }

        double dx = std::abs(adfGeoTransform[1]);
        double dy = std::abs(adfGeoTransform[5]);
        double ulx = adfGeoTransform[0];
        double uly = adfGeoTransform[3];

        int nx = GDALGetRasterXSize(m_rast);
        int ny = GDALGetRasterYSize(m_rast);

        Box box{ulx,
                uly - ny * dy,
                ulx + nx * dx,
                uly};

        m_grid = {box, dx, dy};
    }

    GDALRasterWrapper::GDALRasterWrapper(exactextract::GDALRasterWrapper && src) noexcept :
        m_rast{src.m_rast},
        m_band{src.m_band},
        m_nodata_value{src.m_nodata_value},
        m_has_nodata{src.m_has_nodata},
        m_grid{src.m_grid} {
        src.m_rast = nullptr;
    }

}
