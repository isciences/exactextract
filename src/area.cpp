#include "area.h"

#include <cmath>
#include <cstddef>

double area_signed(const std::vector<Coordinate> & ring) {
    if (ring.size() < 3) {
        return 0;
    }

    double sum{ 0 };

    double x0{ ring[0].x };
    for (size_t i = 1; i < ring.size() - 1; i++) {
        double x = ring[i].x - x0;
        double y1 = ring[i + 1].y;
        double y2 = ring[i - 1].y;
        sum += x * (y2 - y1);
    }

    return sum / 2.0;
}

double area(const std::vector<Coordinate> & ring) {
    return std::abs(area_signed(ring));
}
