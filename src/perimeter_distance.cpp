#include "perimeter_distance.h"

double perimeter_distance(double xmin, double ymin, double xmax, double ymax, double x, double y) {
    if (x == xmin) {
        // Left
        return y - ymin;
    }

    if (y == ymax) {
        // Top
        return (ymax - ymin) + x - xmin;
    }

    if (x == xmax) {
        // Right
        return (xmax - xmin) + (ymax - ymin) + ymax - y;
    }

    if (y == ymin) {
        // Bottom
        return (xmax - xmin) + 2*(ymax - ymin) + (xmax - x);
    }

    throw std::runtime_error("Never get here");
}

double perimeter_distance(double xmin, double ymin, double xmax, double ymax, const Coordinate & c) {
    return perimeter_distance(xmin, ymin, xmax, ymax, c.x, c.y);
}

double perimeter_distance(const Box & b, const Coordinate & c) {
    return perimeter_distance(b.xmin, b.ymin, b.xmax, b.ymax, c.x, c.y);
}

double perimeter_distance_ccw(double measure1, double measure2, double perimeter) {
    if (measure2 <= measure1) {
        return measure1 - measure2;
    }
    return perimeter + measure1 - measure2;
}
