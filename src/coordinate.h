#ifndef COORDINATE_H
#define COORDINATE_H

#include <cmath>
#include <iostream>

struct Coordinate {
    double x;
    double y;

    Coordinate() {}

    Coordinate(double x, double y) : x{x}, y{y} {}

    bool equals(const Coordinate & other, double tol) const {
        return std::abs(other.x - x) < tol && std::abs(other.y - y) < tol;
    }

    bool operator==(const Coordinate & other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const Coordinate & other) const {
        return !(*this == other);
    }
};

std::ostream& operator<< (std::ostream & os, const Coordinate & c);

#endif