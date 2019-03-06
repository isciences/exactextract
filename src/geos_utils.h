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

#ifndef EXACTEXTRACT_GEOS_UTILS_H
#define EXACTEXTRACT_GEOS_UTILS_H

#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

#include <geos_c.h>

#include "box.h"
#include "coordinate.h"
#include "segment_orientation.h"

namespace exactextract {

    using geom_ptr_r = std::unique_ptr<GEOSGeometry, std::function<void(GEOSGeometry*)>>;
    using tree_ptr_r = std::unique_ptr<GEOSSTRtree, std::function<void(GEOSSTRtree*)>>;
    using seq_ptr_r = std::unique_ptr<GEOSCoordSequence, std::function<void(GEOSCoordSequence*)>>;
    using prep_geom_ptr_r = std::unique_ptr<const GEOSPreparedGeometry, std::function<void(const GEOSPreparedGeometry*)>>;

    inline geom_ptr_r geos_ptr(GEOSContextHandle_t context, GEOSGeometry* g) {
        auto deleter = [context](GEOSGeometry* g){ GEOSGeom_destroy_r(context, g); };
        return geom_ptr_r{g, deleter};
    }

    inline tree_ptr_r geos_ptr(GEOSContextHandle_t context, GEOSSTRtree* t) {
        auto deleter = [context](GEOSSTRtree_t* t){ GEOSSTRtree_destroy_r(context, t); };
        return tree_ptr_r{t, deleter};
    }

    inline seq_ptr_r geos_ptr(GEOSContextHandle_t context, GEOSCoordSequence* s) {
        auto deleter = [context](GEOSCoordSequence* s){ GEOSCoordSeq_destroy_r(context, s); };
        return seq_ptr_r{s, deleter};
    }

    inline prep_geom_ptr_r
    GEOSPrepare_ptr(GEOSContextHandle_t context, const GEOSGeometry *g) {
        auto deleter = [context](const GEOSPreparedGeometry* pg) { GEOSPreparedGeom_destroy_r(context, pg); };
        return prep_geom_ptr_r{GEOSPrepare_r(context, g), deleter};
    };

    inline seq_ptr_r
    GEOSCoordSeq_create_ptr(GEOSContextHandle_t context, unsigned int size, unsigned int dims) {
        return geos_ptr(context, GEOSCoordSeq_create_r(context, size, dims));
    };

    inline geom_ptr_r
    GEOSGeom_createPoint_ptr(GEOSContextHandle_t context, double x, double y) {
        auto seq = GEOSCoordSeq_create_ptr(context, 1, 2);
        GEOSCoordSeq_setX_r(context, seq.get(), 0, x);
        GEOSCoordSeq_setY_r(context, seq.get(), 0, y);
        return geos_ptr(context, GEOSGeom_createPoint_r(context, seq.release()));
    };

    inline geom_ptr_r
    GEOSGeom_createLineString_ptr(GEOSContextHandle_t context, GEOSCoordSequence *seq) {
        return geos_ptr(context, GEOSGeom_createLineString_r(context, seq));
    };

    inline unsigned int geos_get_num_points(GEOSContextHandle_t context, const GEOSCoordSequence *s) {
        unsigned int result;
        if (!GEOSCoordSeq_getSize_r(context, s, &result)) {
            throw std::runtime_error("Error calling GEOSCoordSeq_getSize_r.");
        }
        return result;
    }

    inline geom_ptr_r
    GEOSGeom_read_r(GEOSContextHandle_t context, const std::string &s) {
        return geos_ptr(context, GEOSGeomFromWKT_r(context, s.c_str()));
    }
    geom_ptr_r geos_make_box_polygon(GEOSContextHandle_t context, const Box & b);

    Box geos_get_box(GEOSContextHandle_t context, const GEOSGeometry* g);

    bool segment_intersection(GEOSContextHandle_t context, const Coordinate &a0, const Coordinate &a1, const Coordinate &b0, const Coordinate &b1,
                              Coordinate &result);

    bool geos_is_ccw(GEOSContextHandle_t context, const GEOSCoordSequence *s);

    std::vector<Coordinate> read(GEOSContextHandle_t context, const GEOSCoordSequence *s);

    SegmentOrientation initial_segment_orientation(GEOSContextHandle_t context, const GEOSCoordSequence *s);

}

#endif //RASTER_OVERLAY_CPP_GEOS_UTILS_H
