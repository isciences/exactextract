// Copyright (c) 2018-2023 ISciences, LLC.
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

#include <gdal.h>
#include <ogr_srs_api.h>

#include "gdal_raster_wrapper.h"

#include <stdexcept>

namespace exactextract {

    GDALRasterWrapper::GDALRasterWrapper(const std::string &filename, int bandnum) : m_grid{Grid<bounded_extent>::make_empty()} {
        auto rast = GDALOpen(filename.c_str(), GA_ReadOnly);
        if (!rast) {
            throw std::runtime_error("Failed to open " + filename);
        }

        int has_nodata;
        auto band = GDALGetRasterBand(rast, bandnum);
        double nodata_value = GDALGetRasterNoDataValue(band, &has_nodata);

        m_rast = rast;
        m_band = band;
        m_nodata_value = nodata_value;
        m_has_nodata = static_cast<bool>(has_nodata);
        set_name(filename);
        compute_raster_grid();
    }

    
    GDALRasterWrapper::~GDALRasterWrapper() {
        // We can't use a std::unique_ptr because GDALDatasetH is an incomplete type.
        // So we include a destructor and move constructor to manage the resource.
        if (m_rast != nullptr)
            GDALClose(m_rast);
    }

    bool
    GDALRasterWrapper::cartesian() const {
        const auto& srs = GDALGetSpatialRef(m_rast);
        return !OSRIsGeographic(srs);
    }

    std::unique_ptr<AbstractRaster<double>> GDALRasterWrapper::read_box(const Box &box) {
        auto cropped_grid = m_grid.shrink_to_fit(box);
        auto vals = std::make_unique<Raster<double>>(cropped_grid);

        if (m_has_nodata) {
            vals->set_nodata(m_nodata_value);
        }

        auto error = GDALRasterIO(m_band,
                                  GF_Read,
                                  (int) cropped_grid.col_offset(m_grid),
                                  (int) cropped_grid.row_offset(m_grid),
                                  (int) cropped_grid.cols(),
                                  (int) cropped_grid.rows(),
                                  vals->data().data(),
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

    void GDALRasterWrapper::compute_raster_grid() {
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
