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

GDALRasterWrapper::
  GDALRasterWrapper(const std::string& dsn, int bandnum)
  : GDALRasterWrapper(std::make_shared<GDALRaster>(dsn), bandnum)
{
}

GDALRasterWrapper::
  GDALRasterWrapper(std::shared_ptr<GDALRaster> rast, int bandnum)
  : m_rast(rast)
  , m_grid{ Grid<bounded_extent>::make_empty() }
{

    int has_nodata;
    auto band = GDALGetRasterBand(m_rast->get(), bandnum);
    double nodata_value = GDALGetRasterNoDataValue(band, &has_nodata);

    m_band = band;
    m_nodata_value = nodata_value;
    m_has_nodata = static_cast<bool>(has_nodata);
    // set_name(GDALGetDescription(m_rast->get()));
    compute_raster_grid();
    read_scale_and_offset();

    auto mask_flags = GDALGetMaskFlags(m_band);

    m_read_mask = !(mask_flags == GMF_ALL_VALID || (mask_flags == GMF_NODATA && !m_scaled));
    if (m_read_mask) {
        m_has_nodata = false;
    }
}

GDALRasterWrapper::~GDALRasterWrapper()
{
}

void
GDALRasterWrapper::read_scale_and_offset()
{
    int has_scale = 0;
    double scale = 1;
    int has_offset = 0;
    double offset = 0;

    m_scale = GDALGetRasterScale(m_band, &has_scale);
    m_offset = GDALGetRasterOffset(m_band, &has_offset);

    m_scaled = has_scale || has_offset;
}

bool
GDALRasterWrapper::cartesian() const
{
    OGRSpatialReferenceH srs = GDALGetSpatialRef(m_rast->get());
    return srs == nullptr || !OSRIsGeographic(srs);
}

static void
apply_scale_and_offset(double* px, std::size_t size, double scale, double offset)
{
    double* end = px + size;
    while (px < end) {
        *px = *px * scale + offset;
        px++;
    }
}

#define HAVE_GDAL_INT8 (GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 7))
#define HAVE_GDAL_INT64 (GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 5))

RasterVariant
GDALRasterWrapper::read_box(const Box& box)
{
    auto cropped_grid = m_grid.shrink_to_fit(box);
    RasterVariant ret;

    auto band_type = GDALGetRasterDataType(m_band);
    void* buffer;
    GDALDataType read_type;

    if (m_scaled) {
        auto rast = make_raster<double>(cropped_grid);
        buffer = rast->data().data();
        ret = std::move(rast);
        read_type = GDT_Float64;
    } else {
        if (band_type == GDT_Byte) {
            auto rast = make_raster<std::uint8_t>(cropped_grid);
            buffer = rast->data().data();
            ret = std::move(rast);
            read_type = GDT_Byte;
#if HAVE_GDAL_INT8
        } else if (band_type == GDT_Int8) {
            auto rast = make_raster<std::int8_t>(cropped_grid);
            buffer = rast->data().data();
            ret = std::move(rast);
            read_type = GDT_Int8;
#endif
        } else if (band_type == GDT_Int16) {
            auto rast = make_raster<std::int16_t>(cropped_grid);
            buffer = rast->data().data();
            ret = std::move(rast);
            read_type = GDT_Int16;
        } else if (band_type == GDT_UInt16) {
            auto rast = make_raster<std::uint16_t>(cropped_grid);
            buffer = rast->data().data();
            ret = std::move(rast);
            read_type = GDT_UInt16;
        } else if (band_type == GDT_Int32) {
            auto rast = make_raster<std::int32_t>(cropped_grid);
            buffer = rast->data().data();
            ret = std::move(rast);
            read_type = GDT_Int32;
        } else if (band_type == GDT_UInt32) {
            auto rast = make_raster<std::uint32_t>(cropped_grid);
            buffer = rast->data().data();
            ret = std::move(rast);
            read_type = GDT_UInt32;
#if HAVE_GDAL_INT64
        } else if (band_type == GDT_Int64) {
            auto rast = make_raster<std::int64_t>(cropped_grid);
            buffer = rast->data().data();
            ret = std::move(rast);
            read_type = GDT_Int64;
        } else if (band_type == GDT_UInt64) {
            auto rast = make_raster<std::uint64_t>(cropped_grid);
            buffer = rast->data().data();
            ret = std::move(rast);
            read_type = GDT_UInt64;
#endif
        } else if (band_type == GDT_Float32) {
            auto rast = make_raster<float>(cropped_grid);
            buffer = rast->data().data();
            ret = std::move(rast);
            read_type = GDT_Float32;
        } else {
            auto rast = make_raster<double>(cropped_grid);
            buffer = rast->data().data();
            ret = std::move(rast);
            read_type = GDT_Float64;
        }
    }

    if (cropped_grid.empty()) {
        return ret;
    }

    auto error = GDALRasterIO(m_band,
                              GF_Read,
                              (int)cropped_grid.col_offset(m_grid),
                              (int)cropped_grid.row_offset(m_grid),
                              (int)cropped_grid.cols(),
                              (int)cropped_grid.rows(),
                              buffer,
                              (int)cropped_grid.cols(),
                              (int)cropped_grid.rows(),
                              read_type,
                              0,
                              0);

    if (error) {
        throw std::runtime_error("Error reading from raster.");
    }

    if (m_scaled) {
        apply_scale_and_offset(static_cast<double*>(buffer), cropped_grid.size(), m_scale, m_offset);
    }

    if (m_read_mask) {
        GDALRasterBandH mask = GDALGetMaskBand(m_band);
        auto mask_rast = make_raster<std::int8_t>(cropped_grid);
        buffer = mask_rast->data().data();

        error = GDALRasterIO(mask,
                             GF_Read,
                             (int)cropped_grid.col_offset(m_grid),
                             (int)cropped_grid.row_offset(m_grid),
                             (int)cropped_grid.cols(),
                             (int)cropped_grid.rows(),
                             buffer,
                             (int)cropped_grid.cols(),
                             (int)cropped_grid.rows(),
                             GDT_Byte,
                             0,
                             0);

        std::visit([&mask_rast](auto& rast) {
            rast->set_mask(std::move(mask_rast));
        },
                   ret);

        if (error) {
            throw std::runtime_error("Error reading from raster.");
        }
    }

    return ret;
}

void
GDALRasterWrapper::compute_raster_grid()
{
    double adfGeoTransform[6];
    if (GDALGetGeoTransform(m_rast->get(), adfGeoTransform) != CE_None) {
        throw std::runtime_error("Error reading transform");
    }

    double dx = std::abs(adfGeoTransform[1]);
    double dy = std::abs(adfGeoTransform[5]);
    double ulx = adfGeoTransform[0];
    double uly = adfGeoTransform[3];

    int nx = GDALGetRasterXSize(m_rast->get());
    int ny = GDALGetRasterYSize(m_rast->get());

    Box box{ ulx,
             uly - ny * dy,
             ulx + nx * dx,
             uly };

    m_grid = { box, dx, dy };
}

OGRSpatialReferenceH
GDALRasterWrapper::srs() const
{
    return GDALGetSpatialRef(m_rast->get());
}

}
