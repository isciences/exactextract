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

#include <memory>

#include <geos_c.h>

#include "extent.h"
#include "floodfill.h"
#include "geos_utils.h"
#include "matrix.h"

namespace exactextract {

    FloodFill::FloodFill(const GEOSGeometry *g, const Extent &extent) :
            m_g{nullptr, GEOSGeom_destroy},
            m_pg{nullptr, GEOSPreparedGeom_destroy},
            m_extent{extent} {
        geom_ptr ring_copy{GEOSGeom_clone(g), GEOSGeom_destroy};
        m_g = {GEOSGeom_createPolygon(ring_copy.release(), nullptr, 0), GEOSGeom_destroy};
        m_pg = {GEOSPrepare(m_g.get()), GEOSPreparedGeom_destroy};
    }

    bool FloodFill::cell_is_inside(size_t i, size_t j) const {
        double x = m_extent.x_for_col(j);
        double y = m_extent.y_for_row(i);

        auto point = GEOSGeom_createPoint_ptr(x, y);

        return GEOSPreparedContains(m_pg.get(), point.get());
    }

}
