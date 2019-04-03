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

#include "geos_utils.h"

namespace exactextract {

    geom_ptr_r geos_make_box_polygon(GEOSContextHandle_t context, const Box & b) {
        auto seq = geos_ptr(context, GEOSCoordSeq_create_r(context, 5, 2));

        GEOSCoordSeq_setX_r(context, seq.get(), 0, b.xmin);
        GEOSCoordSeq_setY_r(context, seq.get(), 0, b.ymin);

        GEOSCoordSeq_setX_r(context, seq.get(), 1, b.xmax);
        GEOSCoordSeq_setY_r(context, seq.get(), 1, b.ymin);

        GEOSCoordSeq_setX_r(context, seq.get(), 2, b.xmax);
        GEOSCoordSeq_setY_r(context, seq.get(), 2, b.ymax);

        GEOSCoordSeq_setX_r(context, seq.get(), 3, b.xmin);
        GEOSCoordSeq_setY_r(context, seq.get(), 3, b.ymax);

        GEOSCoordSeq_setX_r(context, seq.get(), 4, b.xmin);
        GEOSCoordSeq_setY_r(context, seq.get(), 4, b.ymin);

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
    }

    Box geos_get_box(GEOSContextHandle_t context, const GEOSGeometry* g) {
        double xmin, ymin, xmax, ymax;

        if (!(GEOSGeom_getXMin_r(context, g, &xmin) &&
              GEOSGeom_getYMin_r(context, g, &ymin) &&
              GEOSGeom_getXMax_r(context, g, &xmax) &&
              GEOSGeom_getYMax_r(context, g, &ymax))) {
            throw std::runtime_error("Error getting geometry extent.");
        }

        return {xmin, ymin, xmax, ymax};
    }

    bool geos_is_ccw(GEOSContextHandle_t context, const GEOSCoordSequence *s) {
        char result;
        if (!GEOSCoordSeq_isCCW_r(context, s, &result)) {
            throw std::runtime_error("Error calling GEOSCoordSeq_isCCW_r.");
        }
        return result;
    }

    std::vector<Coordinate> read(GEOSContextHandle_t context, const GEOSCoordSequence *s) {
        unsigned int size;

        if (!GEOSCoordSeq_getSize_r(context, s, &size)) {
            throw std::runtime_error("Error calling GEOSCoordSeq_getSize.");
        }

        std::vector<Coordinate> coords{size};

        for (unsigned int i = 0; i < size; i++) {
            if (!GEOSCoordSeq_getX_r(context, s, i, &(coords[i].x)) || !GEOSCoordSeq_getY_r(context, s, i, &(coords[i].y))) {
                throw std::runtime_error("Error reading coordinates.");
            }
        }

        return coords;
    }

    SegmentOrientation initial_segment_orientation(GEOSContextHandle_t context, const GEOSCoordSequence *s) {
        double x0, y0;
        double xn, yn;
        unsigned int size;

        if (!GEOSCoordSeq_getSize_r(context, s, &size)) {
            throw std::runtime_error("Error calling GEOSCoordSeq_getSize.");
        }

        if (!GEOSCoordSeq_getX_r(context, s, 0, &x0) || !GEOSCoordSeq_getY_r(context, s, 0, &y0)) {
            throw std::runtime_error("Error reading coordinates.");
        }

        for (unsigned int i = 1; i < size; i++) {
            if (!GEOSCoordSeq_getX_r(context, s, i, &xn) || !GEOSCoordSeq_getY_r(context, s, i, &yn)) {
                throw std::runtime_error("Error reading coordinates.");
            }

            if (xn != x0 || yn != y0) {
                if (xn == x0) {
                    if (yn > y0) {
                        return SegmentOrientation::VERTICAL_UP;
                    } else {
                        return SegmentOrientation::VERTICAL_DOWN;
                    }
                }
                if (yn == y0) {
                    if (xn > x0) {
                        return SegmentOrientation::HORIZONTAL_RIGHT;
                    } else {
                        return SegmentOrientation::HORIZONTAL_LEFT;
                    }
                }

                return SegmentOrientation::ANGLED;
            }
        }

        throw std::runtime_error("Couldn't find segment orientation.");
    }

}