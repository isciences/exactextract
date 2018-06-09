#include "geos_utils.h"

namespace exactextract {

    geom_ptr geos_make_box_polygon(double x0, double y0, double x1, double y1) {
        seq_ptr seq{GEOSCoordSeq_create(5, 2), GEOSCoordSeq_destroy};

        GEOSCoordSeq_setX(seq.get(), 0, x0);
        GEOSCoordSeq_setY(seq.get(), 0, y0);

        GEOSCoordSeq_setX(seq.get(), 1, x1);
        GEOSCoordSeq_setY(seq.get(), 1, y0);

        GEOSCoordSeq_setX(seq.get(), 2, x1);
        GEOSCoordSeq_setY(seq.get(), 2, y1);

        GEOSCoordSeq_setX(seq.get(), 3, x0);
        GEOSCoordSeq_setY(seq.get(), 3, y1);

        GEOSCoordSeq_setX(seq.get(), 4, x0);
        GEOSCoordSeq_setY(seq.get(), 4, y0);

        geom_ptr shell{GEOSGeom_createLinearRing(seq.release()), GEOSGeom_destroy};

        return {GEOSGeom_createPolygon(shell.release(), nullptr, 0), GEOSGeom_destroy};
    }

    bool segment_intersection(const Coordinate &a0, const Coordinate &a1, const Coordinate &b0, const Coordinate &b1,
                              Coordinate &result) {
        auto seqa = GEOSCoordSeq_create_ptr(2, 2);
        auto seqb = GEOSCoordSeq_create_ptr(2, 2);

        GEOSCoordSeq_setX(seqa.get(), 0, a0.x);
        GEOSCoordSeq_setY(seqa.get(), 0, a0.y);
        GEOSCoordSeq_setX(seqa.get(), 1, a1.x);
        GEOSCoordSeq_setY(seqa.get(), 1, a1.y);

        GEOSCoordSeq_setX(seqb.get(), 0, b0.x);
        GEOSCoordSeq_setY(seqb.get(), 0, b0.y);
        GEOSCoordSeq_setX(seqb.get(), 1, b1.x);
        GEOSCoordSeq_setY(seqb.get(), 1, b1.y);

        auto geom_a = GEOSGeom_createLineString_ptr(seqa.release());
        auto geom_b = GEOSGeom_createLineString_ptr(seqb.release());

        geom_ptr intersection{GEOSIntersection(geom_a.get(), geom_b.get()), GEOSGeom_destroy};


        if (GEOSisEmpty(intersection.get())) {
            return false;
        }

        if (GEOSGeomTypeId(intersection.get()) != GEOS_POINT) {
            return false;
        }

        GEOSGeomGetX(intersection.get(), &result.x);
        GEOSGeomGetY(intersection.get(), &result.y);

        return true;
    }

    Box geos_get_box(const GEOSGeometry *g) {
        double xmin, ymin, xmax, ymax;
#if HAVE_370
        if (!(GEOSGeom_getXMin(g, &xmin) &&
              GEOSGeom_getYMin(g, &ymin) &&
              GEOSGeom_getXMax(g, &xmax) &&
              GEOSGeom_getYMax(g, &ymax))) {
            throw std::runtime_error("Error getting geometry extent.");
        }
#else
        geom_ptr env { GEOSEnvelope(g), GEOSGeom_destroy };

        const GEOSGeometry* ring = GEOSGetExteriorRing(env.get());
        const GEOSCoordSequence* seq = GEOSGeom_getCoordSeq(ring);

        xmin = std::numeric_limits<double>::max();
        ymin = std::numeric_limits<double>::max();
        xmax = std::numeric_limits<double>::lowest();
        ymax = std::numeric_limits<double>::lowest();

        for (unsigned int i = 0; i < 4; i++) {
            double x, y;

            if (!GEOSCoordSeq_getX(seq, i, &x) || !GEOSCoordSeq_getY(seq, i, &y)) {
                throw std::runtime_error("Error reading coordinates.");
            }

            xmin = std::min(xmin, x);
            ymin = std::min(ymin, y);
            xmax = std::max(xmax, x);
            ymax = std::max(ymax, y);
        }
#endif
        return {xmin, ymin, xmax, ymax};
    };

    bool geos_is_ccw(const GEOSCoordSequence *s) {
#if HAVE_370
        char result;
        if (!GEOSCoordSeq_isCCW(s, &result)) {
            throw std::runtime_error("Error calling GEOSCoordSeq_isCCW.");
        }
        return result;
#else
        std::vector<Coordinate> coords = read(s);

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

        int disc = GEOSOrientationIndex(a.x, a.y, b.x, b.y, c.x, c.y);

        if (disc == 0) {
            // poly is CCW if prev x is right of next x
            return (a.x > b.x);
        } else {
            // if area is positive, points are ordered CCW
            return disc > 0;
        }
#endif
    }

    std::vector<Coordinate> read(const GEOSCoordSequence *s) {
        unsigned int size;

        if (!GEOSCoordSeq_getSize(s, &size)) {
            throw std::runtime_error("Error calling GEOSCoordSeq_getSize.");
        }

        std::vector<Coordinate> coords{size};

        for (unsigned int i = 0; i < size; i++) {
            if (!GEOSCoordSeq_getX(s, i, &(coords[i].x)) || !GEOSCoordSeq_getY(s, i, &(coords[i].y))) {
                throw std::runtime_error("Error reading coordinates.");
            }
        }

        return coords;
    }

    SegmentOrientation initial_segment_orientation(const GEOSCoordSequence *s) {
        double x0, y0;
        double xn, yn;
        unsigned int size;

        if (!GEOSCoordSeq_getSize(s, &size)) {
            throw std::runtime_error("Error calling GEOSCoordSeq_getSize.");
        }

        if (!GEOSCoordSeq_getX(s, 0, &x0) || !GEOSCoordSeq_getY(s, 0, &y0)) {
            throw std::runtime_error("Error reading coordinates.");
        }

        for (unsigned int i = 1; i < size; i++) {
            if (!GEOSCoordSeq_getX(s, i, &xn) || !GEOSCoordSeq_getY(s, i, &yn)) {
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
    }

}