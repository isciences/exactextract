#ifndef BOX_H
#define BOX_H

#include "coordinate.h"
#include "crossing.h"
#include "side.h"

namespace exactextract {

    struct Box {
        double xmin;
        double ymin;
        double xmax;
        double ymax;

        Box(double xmin, double ymin, double xmax, double ymax) :
                xmin{xmin},
                ymin{ymin},
                xmax{xmax},
                ymax{ymax} {}

        double width() const {
            return xmax - xmin;
        }

        double height() const {
            return ymax - ymin;
        }

        double area() const {
            return width() * height();
        }

        double perimeter() const {
            return 2 * width() + 2 * height();
        }

        Coordinate upper_left() const {
            return Coordinate{xmin, ymax};
        }

        Coordinate upper_right() const {
            return Coordinate{xmax, ymax};
        }

        Coordinate lower_left() const {
            return Coordinate{xmin, ymin};
        }

        Coordinate lower_right() const {
            return Coordinate{xmax, ymin};
        }

        Side side(const Coordinate &c) const;

        Crossing crossing(const Coordinate &c1, const Coordinate &c2) const;

        bool contains(const Coordinate &c) const;

        bool strictly_contains(const Coordinate &c) const;

    };

}

#endif
