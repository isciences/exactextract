#ifndef RASTER_OVERLAY_CPP_GEOS_UTILS_H
#define RASTER_OVERLAY_CPP_GEOS_UTILS_H

#include <algorithm>
#include <limits>
#include <memory>
#include <vector>

#include <geos_c.h>

#define HAVE_370 (GEOS_VERSION_MAJOR >= 3 && GEOS_VERSION_MINOR >= 7)

#include "box.h"
#include "coordinate.h"
#include "segment_orientation.h"

using seq_ptr = std::unique_ptr<GEOSCoordSequence, decltype(&GEOSCoordSeq_destroy)>;
using geom_ptr = std::unique_ptr<GEOSGeometry, decltype(&GEOSGeom_destroy)>;
using prep_geom_ptr = std::unique_ptr<const GEOSPreparedGeometry, decltype(&GEOSPreparedGeom_destroy)>;

inline prep_geom_ptr
GEOSPrepare_ptr(const GEOSGeometry* g)
{
    return { GEOSPrepare(g), GEOSPreparedGeom_destroy };
};

inline seq_ptr
GEOSCoordSeq_create_ptr(unsigned int size, unsigned int dims)
{
    return { GEOSCoordSeq_create(size, dims), GEOSCoordSeq_destroy };
};

inline geom_ptr
GEOSGeom_createPoint_ptr(GEOSCoordSequence* seq)
{
    return { GEOSGeom_createPoint(seq), GEOSGeom_destroy };
};

inline geom_ptr
GEOSGeom_read(const std::string & s) {
    return { GEOSGeomFromWKT(s.c_str()), GEOSGeom_destroy };
}

inline geom_ptr
GEOSGeom_createPoint_ptr(double x, double y)
{
    auto seq = GEOSCoordSeq_create_ptr(1, 2);
    GEOSCoordSeq_setX(seq.get(), 0, x);
    GEOSCoordSeq_setY(seq.get(), 0, y);
    return { GEOSGeom_createPoint(seq.release()), GEOSGeom_destroy };
};

inline geom_ptr
GEOSGeom_createLineString_ptr(GEOSCoordSequence*  seq)
{
    return { GEOSGeom_createLineString(seq), GEOSGeom_destroy };
};

inline unsigned int geos_get_num_points(const GEOSCoordSequence* s)
{
    unsigned int result;
    if (!GEOSCoordSeq_getSize(s, &result)) {
        throw std::runtime_error("Error calling GEOSCoordSeq_getSize.");
    }
    return result;
}

geom_ptr geos_make_box_polygon(double x0, double y0, double x1, double y1);
Box geos_get_box(const GEOSGeometry *g);
bool segment_intersection(const Coordinate & a0, const Coordinate & a1, const Coordinate & b0, const Coordinate & b1, Coordinate & result);
bool geos_is_ccw(const GEOSCoordSequence* s);
std::vector<Coordinate> read(const GEOSCoordSequence* s);
SegmentOrientation initial_segment_orientation(const GEOSCoordSequence* s);

#endif //RASTER_OVERLAY_CPP_GEOS_UTILS_H
