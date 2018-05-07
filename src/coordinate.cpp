#include "coordinate.h"

std::ostream& operator<< (std::ostream & os, const Coordinate & c) {
    os << "POINT (" << c.x << " " << c.y << ")";
    return os;
}