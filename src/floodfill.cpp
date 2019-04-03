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

#include "grid.h"
#include "floodfill.h"
#include "geos_utils.h"
#include "matrix.h"

namespace exactextract {

    FloodFill::FloodFill(GEOSContextHandle_t context, const GEOSGeometry *g, const Grid<bounded_extent> &extent) :
            m_extent{extent},
            m_geos_context{context},
            m_g{nullptr},
            m_pg{nullptr} {
        geom_ptr_r ring_copy = geos_ptr(context, GEOSGeom_clone_r(context, g));
        m_g = geos_ptr(context, GEOSGeom_createPolygon_r(context, ring_copy.release(), nullptr, 0));
        m_pg = GEOSPrepare_ptr(context, m_g.get());
    }

    bool FloodFill::cell_is_inside(size_t i, size_t j) const {
        double x = m_extent.x_for_col(j);
        double y = m_extent.y_for_row(i);

        auto point = GEOSGeom_createPoint_ptr(m_geos_context, x, y);

        return GEOSPreparedContains_r(m_geos_context, m_pg.get(), point.get());
    }

}
