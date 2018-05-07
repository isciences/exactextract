#ifndef CELL_H
#define CELL_H

#include "box.h"
#include "crossing.h"
#include "coordinate.h"
#include "side.h"
#include "traversal.h"

class Cell {

    public:

    Cell(double xmin, double ymin, double xmax, double ymax) :
        m_box{xmin, ymin, xmax, ymax} {}

    void force_exit();

    double width() const;
    double height() const;
    double area() const;

    double covered_fraction() const;

    Traversal& last_traversal();
    const Box& box() const { return m_box; }

    bool take(const Coordinate & c);

    private:
        enum class Location {
            INSIDE, OUTSIDE, BOUNDARY
        };

    Box m_box;

    std::vector<Traversal> m_traversals;

    Side side(const Coordinate & c) const;

    Location location(const Coordinate & c) const;

    Traversal& traversal_in_progress();

    friend std::ostream& operator<< (std::ostream & os, const Cell & c);
};

std::ostream& operator<< (std::ostream & os, const Cell & c);

#endif