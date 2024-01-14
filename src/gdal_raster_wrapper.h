// Copyright (c) 2018-2024 ISciences, LLC.
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

#include "box.h"
#include "grid.h"
#include "raster.h"
#include "raster_source.h"

#include <gdal.h>

namespace exactextract {

class GDALRaster
{
  public:
    GDALRaster(const std::string& dsn)
      : m_dataset(GDALOpen(dsn.c_str(), GA_ReadOnly))
    {
        if (m_dataset == nullptr) {
            throw std::runtime_error("Failed to read raster: " + dsn);
        }
    }

    ~GDALRaster()
    {
        if (m_dataset != nullptr) {
            GDALClose(m_dataset);
        }
    }

    GDALDatasetH get()
    {
        return m_dataset;
    }

  private:
    GDALDatasetH m_dataset;
};

class GDALRasterWrapper : public RasterSource
{

  public:
    GDALRasterWrapper(std::shared_ptr<GDALRaster> rast, int bandnum);

    GDALRasterWrapper(const std::string& filename, int bandnum);

    const Grid<bounded_extent>& grid() const override
    {
        return m_grid;
    }

    RasterVariant read_box(const Box& box) override;

    ~GDALRasterWrapper() override;

    bool cartesian() const;

  private:
    std::shared_ptr<GDALRaster> m_rast;
    GDALRasterBandH m_band;
    double m_nodata_value;
    bool m_has_nodata;
    Grid<bounded_extent> m_grid;

    void compute_raster_grid();

    template<typename T>
    std::unique_ptr<Raster<T>> make_raster(const Grid<bounded_extent>& grid)
    {
        auto ret = std::make_unique<Raster<T>>(grid);
        if (m_has_nodata) {
            ret->set_nodata(m_nodata_value);
        }
        return ret;
    }
};
}
