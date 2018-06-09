#include "coordinate.h"

namespace exactextract {

    std::ostream &operator<<(std::ostream &os, const Coordinate &c) {
        os << "POINT (" << c.x << " " << c.y << ")";
        return os;
    }

}