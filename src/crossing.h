#ifndef CELL_CROSSING_H
#define CELL_CROSSING_H

#include "coordinate.h"
#include "side.h"

class Crossing {
    public:
        Crossing(Side s, double x, double y) : m_side{s}, m_coord{x, y} {}
        Crossing(Side s, const Coordinate & c) : m_side{s}, m_coord{c} {}

        const Side& side() const {
            return m_side;
        }

        const Coordinate& coord() const {
            return m_coord;
        }

    private:
        Side m_side;
        Coordinate m_coord;

};

#endif