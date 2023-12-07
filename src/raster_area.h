// Copyright (c) 2021 ISciences, LLC.
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

#ifndef EXACTEXTRACT_RASTER_AREA_H
#define EXACTEXTRACT_RASTER_AREA_H

#include "raster.h"

namespace exactextract {
template<typename T>
class CartesianAreaRaster : public AbstractRaster<T>
{
  public:
    explicit CartesianAreaRaster(const Grid<bounded_extent>& ex)
      : AbstractRaster<T>(ex)
      , m_area(grid_cell(AbstractRaster<T>::grid(), 0, 0).area())
    {
    }

    T operator()(size_t row, size_t column) const override
    {
        (void)row;
        (void)column;
        return m_area;
    }

  private:
    double m_area;
};

template<typename T>
class SphericalAreaRaster : public AbstractRaster<T>
{
  public:
    explicit SphericalAreaRaster(const Grid<bounded_extent>& ex)
      : AbstractRaster<T>(ex)
      , m_areas(ex.rows())
    {
        const auto& g = AbstractRaster<T>::grid();

        double dlon = g.dx();
        double dlat = g.dy();

        for (size_t i = 0; i < g.rows(); i++) {
            double y = g.y_for_row(i);
            double ymin = y - 0.5 * dlat;
            double ymax = y + 0.5 * dlat;

            m_areas[i] = EARTH_RADIUS_SQ * PI_180 * std::abs(std::sin(ymin * PI_180) - std::sin(ymax * PI_180)) * dlon;
        }
    }

    T operator()(size_t row, size_t column) const override
    {
        (void)column;
        return m_areas[row];
    }

  private:
    static constexpr double EARTH_RADIUS = 6378137;
    static constexpr double EARTH_RADIUS_SQ = EARTH_RADIUS * EARTH_RADIUS;
    static constexpr double PI_180 = 3.141592653589793238462643383279502884197169399375105 / 180;

    std::vector<double> m_areas;
};
}

#endif // EXACTEXTRACT_RASTER_AREA_H
