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

#include "raster.h"
#include "raster_source.h"

namespace exactextract {

class MemoryRasterSource : public RasterSource
{

  public:
    MemoryRasterSource(RasterSource& rs, const Box& extent)
      : m_raster(rs.read_box(extent))
      , m_grid(std::visit([](const auto& r) {
          return r->grid();
      },
                          m_raster))
    {
    }

    MemoryRasterSource(RasterVariant r)
      : m_raster(std::move(r))
      , m_grid(std::visit([](const auto& rp) {
          return rp->grid();
      },
                          m_raster))
    {
    }

    const Grid<bounded_extent>& grid() const override
    {
        return m_grid;
    }

    RasterVariant read_box(const Box& extent) override
    {
        auto cropped_grid = m_grid.crop(extent);

        return std::visit([this, &cropped_grid](const auto& r) {
            return RasterVariant(make_view(*r, cropped_grid));
        },
                          m_raster);
    }

  private:
    template<typename T>
    std::unique_ptr<RasterView<T>> make_view(const AbstractRaster<T>& r, const Grid<bounded_extent>& extent)
    {
        return std::make_unique<RasterView<T>>(r, extent);
    }

    RasterVariant m_raster;
    Grid<bounded_extent> m_grid;
};

}
