#ifndef PERIMETERDISTANCE_H
#define PERIMETERDISTANCE_H

#include "box.h"
#include "coordinate.h"

namespace exactextract {

    double perimeter_distance(double xmin, double ymin, double xmax, double ymax, double x, double y);

    double perimeter_distance(double xmin, double ymin, double xmax, double ymax, const Coordinate &c);

    double perimeter_distance(const Box &b, const Coordinate &c);

    double perimeter_distance_ccw(double measure1, double measure2, double perimeter);

}

#endif