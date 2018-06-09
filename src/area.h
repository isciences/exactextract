#ifndef AREA_H
#define AREA_H

#include <cmath>
#include <vector>
#include <cstddef>

#include "coordinate.h"

namespace exactextract {

    double area_signed(const std::vector<Coordinate> &ring);

    double area(const std::vector<Coordinate> &ring);

}

#endif