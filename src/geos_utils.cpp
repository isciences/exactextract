// Copyright (c) 2018-2020 ISciences, LLC.
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

#include "geos_utils.h"

#include <stdexcept>

#if !HAVE_380
static inline int GEOSCoordSeq_setXY_r(GEOSContextHandle_t context,
        GEOSCoordSequence* seq,
        unsigned int idx,
        double x,
        double y) {
    return GEOSCoordSeq_setX_r(context, seq, idx, x) && GEOSCoordSeq_setY_r(context, seq, idx, y);
}
#endif

namespace exactextract {

    geom_ptr_r geos_make_box_polygon(GEOSContextHandle_t context, const Box & b) {
        auto seq = geos_ptr(context, GEOSCoordSeq_create_r(context, 5, 2));

        GEOSCoordSeq_setXY_r(context, seq.get(), 0, b.xmin, b.ymin);
        GEOSCoordSeq_setXY_r(context, seq.get(), 1, b.xmax, b.ymin);
        GEOSCoordSeq_setXY_r(context, seq.get(), 2, b.xmax, b.ymax);
        GEOSCoordSeq_setXY_r(context, seq.get(), 3, b.xmin, b.ymax);
        GEOSCoordSeq_setXY_r(context, seq.get(), 4, b.xmin, b.ymin);

        auto shell = geos_ptr(context, GEOSGeom_createLinearRing_r(context, seq.release()));

        return geos_ptr(context, GEOSGeom_createPolygon_r(context, shell.release(), nullptr, 0));
    }

    bool segment_intersection(
            GEOSContextHandle_t context,
            const Coordinate &a0,
            const Coordinate &a1,
            const Coordinate &b0,
            const Coordinate &b1,
            Coordinate &result) {
#if HAVE_370
        int code = GEOSSegmentIntersection_r(context,
                                             a0.x, a0.y,
                                             a1.x, a1.y,
                                             b0.x, b0.y,
                                             b1.x, b1.y,
                                             &result.x, &result.y);
        if (!code) {
            throw std::runtime_error("Error in GEOSSegmentIntersection_r");
        }

        return code == 1;
#else
        auto seqa = GEOSCoordSeq_create_ptr(context, 2, 2);
        auto seqb = GEOSCoordSeq_create_ptr(context, 2, 2);

        GEOSCoordSeq_setX_r(context, seqa.get(), 0, a0.x);
        GEOSCoordSeq_setY_r(context, seqa.get(), 0, a0.y);
        GEOSCoordSeq_setX_r(context, seqa.get(), 1, a1.x);
        GEOSCoordSeq_setY_r(context, seqa.get(), 1, a1.y);

        GEOSCoordSeq_setX_r(context, seqb.get(), 0, b0.x);
        GEOSCoordSeq_setY_r(context, seqb.get(), 0, b0.y);
        GEOSCoordSeq_setX_r(context, seqb.get(), 1, b1.x);
        GEOSCoordSeq_setY_r(context, seqb.get(), 1, b1.y);

        auto geom_a = GEOSGeom_createLineString_ptr(context, seqa.release());
        auto geom_b = GEOSGeom_createLineString_ptr(context, seqb.release());

        geom_ptr_r intersection = geos_ptr(context, GEOSIntersection_r(context, geom_a.get(), geom_b.get()));

        if (GEOSisEmpty_r(context, intersection.get())) {
            return false;
        }

        if (GEOSGeomTypeId_r(context, intersection.get()) != GEOS_POINT) {
            return false;
        }

        GEOSGeomGetX_r(context, intersection.get(), &result.x);
        GEOSGeomGetY_r(context, intersection.get(), &result.y);

        return true;
#endif
    }

    Box geos_get_box(GEOSContextHandle_t context, const GEOSGeometry* g) {
        double xmin, ymin, xmax, ymax;

#if HAVE_370
        if (!(GEOSGeom_getXMin_r(context, g, &xmin) &&
              GEOSGeom_getYMin_r(context, g, &ymin) &&
              GEOSGeom_getXMax_r(context, g, &xmax) &&
              GEOSGeom_getYMax_r(context, g, &ymax))) {
            throw std::runtime_error("Error getting geometry extent.");
        }
#else
        xmin = std::numeric_limits<double>::max();
        ymin = std::numeric_limits<double>::max();
        xmax = std::numeric_limits<double>::lowest();
        ymax = std::numeric_limits<double>::lowest();

        geom_ptr_r env = geos_ptr(context, GEOSEnvelope_r(context, g));
        int dim = GEOSGeom_getDimensions_r(context, g);

        const GEOSCoordSequence* seq;
        if (dim == 2) {
            const GEOSGeometry* ring = GEOSGetExteriorRing_r(context, env.get());
            seq = GEOSGeom_getCoordSeq_r(context, ring);
        } else {
            seq = GEOSGeom_getCoordSeq_r(context, g);
        }

        unsigned int npts;
        GEOSCoordSeq_getSize_r(context, seq, &npts);

        for (unsigned int i = 0; i < npts; i++) {
            double x, y;

            if (!GEOSCoordSeq_getX_r(context, seq, i, &x) || !GEOSCoordSeq_getY_r(context, seq, i, &y)) {
                throw std::runtime_error("Error reading coordinates.");
            }

            xmin = std::min(xmin, x);
            ymin = std::min(ymin, y);
            xmax = std::max(xmax, x);
            ymax = std::max(ymax, y);
        }
#endif
        return {xmin, ymin, xmax, ymax};
    }

    std::vector<Box> geos_get_component_boxes(GEOSContextHandle_t context, const GEOSGeometry* g) {
        size_t n = static_cast<size_t>(GEOSGetNumGeometries_r(context, g));
        std::vector<Box> boxes;
        boxes.reserve(n);

        for (size_t i = 0; i < n; i++) {
            boxes.push_back(geos_get_box(context, GEOSGetGeometryN_r(context, g, i)));
        }

        return boxes;
    }

    bool geos_is_ccw(GEOSContextHandle_t context, const GEOSCoordSequence *s) {
#if HAVE_370
        char result;
        if (!GEOSCoordSeq_isCCW_r(context, s, &result)) {
            throw std::runtime_error("Error calling GEOSCoordSeq_isCCW_r.");
        }
        return result;
#else
        std::vector<Coordinate> coords = read(context, s);

        if (coords.size() < 4) {
            throw std::runtime_error("Ring has fewer than 4 points, so orientation cannot be determined.");
        }

        // find highest point
        size_t hi_index = (size_t) std::distance(
                coords.begin(),
                std::max_element(coords.begin(), coords.end(), [](const auto& a, const auto&b) {
                    return a.y < b.y;
                })
        );

        // find distinct point before highest point
        size_t i_prev = hi_index;
        do {
            if (i_prev == 0) {
                i_prev = coords.size() - 1;
            } else {
                i_prev--;
            }
        } while (i_prev != hi_index && coords[i_prev] == coords[hi_index]);

        // find distinct point after highest point
        size_t i_next = hi_index;
        do {
            i_next = (i_next + 1) % coords.size();
        } while (i_next != hi_index && coords[i_next] == coords[hi_index]);

        Coordinate& a = coords[i_prev];
        Coordinate& b = coords[hi_index];
        Coordinate& c = coords[i_next];

        if (a == b || b == c || a == c) {
            return false;
        }

        int disc = GEOSOrientationIndex_r(context, a.x, a.y, b.x, b.y, c.x, c.y);

        if (disc == 0) {
            // poly is CCW if prev x is right of next x
            return (a.x > b.x);
        } else {
            // if area is positive, points are ordered CCW
            return disc > 0;
        }
#endif
    }

    std::vector<Coordinate> read(GEOSContextHandle_t context, const GEOSCoordSequence *s) {
        unsigned int size;

        if (!GEOSCoordSeq_getSize_r(context, s, &size)) {
            throw std::runtime_error("Error calling GEOSCoordSeq_getSize.");
        }

        std::vector<Coordinate> coords{size};

#if HAVE_3100
        if (!GEOSCoordSeq_copyToBuffer_r(context, s, reinterpret_cast<double*>(coords.data()), false, false)) {
            throw std::runtime_error("Error reading coordinates.");
        }
#elif HAVE_380
        for (unsigned int i = 0; i < size; i++) {
            if (!GEOSCoordSeq_getXY_r(context, s, i, &(coords[i].x), &(coords[i].y))) {
                throw std::runtime_error("Error reading coordinates.");
            }
        }
#else
        for (unsigned int i = 0; i < size; i++) {
            if (!GEOSCoordSeq_getX_r(context, s, i, &(coords[i].x)) || !GEOSCoordSeq_getY_r(context, s, i, &(coords[i].y))) {
                throw std::runtime_error("Error reading coordinates.");
            }
        }
#endif
        return coords;
    }

}
