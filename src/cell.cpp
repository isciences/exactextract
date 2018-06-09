#include "area.h"
#include "cell.h"
#include "crossing.h"
#include "traversal_areas.h"
#include "geos_utils.h"

namespace exactextract {

    double Cell::height() const {
        return m_box.height();
    }

    double Cell::width() const {
        return m_box.width();
    }

    double Cell::area() const {
        return m_box.area();
    }

    Side Cell::side(const Coordinate &c) const {
        if (c.x == m_box.xmin) {
            return Side::LEFT;
        } else if (c.x == m_box.xmax) {
            return Side::RIGHT;
        } else if (c.y == m_box.ymin) {
            return Side::BOTTOM;
        } else if (c.y == m_box.ymax) {
            return Side::TOP;
        }

        return Side::NONE;
    }

    void Cell::force_exit() {
        if (last_traversal().exited()) {
            return;
        }

        const Coordinate &last = last_traversal().last_coordinate();

        if (location(last) == Location::BOUNDARY) {
            last_traversal().force_exit(side(last));
        }
    }

    Cell::Location Cell::location(const Coordinate &c) const {
        if (m_box.strictly_contains(c)) {
            return Cell::Location::INSIDE;
        }

        if (m_box.contains(c)) {
            return Cell::Location::BOUNDARY;
        }

        return Cell::Location::OUTSIDE;
    }

    Traversal &Cell::traversal_in_progress() {
        if (m_traversals.empty() || m_traversals[m_traversals.size() - 1].exited()) {
            m_traversals.emplace_back();
        }

        return m_traversals[m_traversals.size() - 1];
    }

    Traversal &Cell::last_traversal() {
        return m_traversals.at(m_traversals.size() - 1);
    }

    bool Cell::take(const Coordinate &c) {
        Traversal &t = traversal_in_progress();

        if (t.empty()) {
            //std::cout << "Entering " << *this << " from " << side(c) << " at " << c << std::endl;

            t.enter(c, side(c));
            return true;
        }

        if (location(c) != Cell::Location::OUTSIDE) {
            //std::cout << "Still in " << *this << " with " << c << std::endl;

            t.add(c);
            return true;
        }

        Crossing x = m_box.crossing(t.last_coordinate(), c);
        t.exit(x.coord(), x.side());

        //std::cout << "Leaving " << *this << " from " << x.side() << " at " << x.coord();
        //std::cout << " on the way to " << c << std::endl;

        return false;
    }

    double Cell::covered_fraction() const {
        // Handle the special case of a ring that is enclosed within a
        // single pixel of our raster
        if (m_traversals.size() == 1 && m_traversals[0].is_closed_ring()) {
            return exactextract::area(m_traversals[0].coords()) / area();
        }

        // TODO consider porting in simplified single-traversal area calculations
        // from Java code. Do they really make a performance difference?
        //if (m_traversals.size() == 1) {
        //    double a = area();

        //    return (a - area_right_of(m_traversals.at(m_traversals.size() - 1))) / a;
        //}

        std::vector<const std::vector<Coordinate> *> coord_lists;

        for (const auto &t : m_traversals) {
            if (!t.traversed() || !t.multiple_unique_coordinates()) {
                continue;
            }

            coord_lists.push_back(&t.coords());
        }

        return left_hand_area(m_box, coord_lists) / area();
    }

#if 0
    Crossing Cell::crossing(const Coordinate & c1, const Coordinate & c2) const {
        Coordinate result(0, 0);

        if (c2.y > c1.y && segment_intersection(c1, c2, m_box.upper_left(), m_box.upper_right(), result)) {
            return Crossing(Side::TOP, result);
        }

        if (c2.y < c1.y && segment_intersection(c1, c2, m_box.lower_right(), m_box.lower_left(), result)) {
            return Crossing(Side::BOTTOM, result);
        }

        if (c2.x < c1.x && segment_intersection(c1, c2, m_box.lower_left(), m_box.upper_left(), result)) {
            return Crossing(Side::LEFT, result);
        }

        if (c2.x > c1.x && segment_intersection(c1, c2, m_box.lower_right(), m_box.upper_right(), result)) {
            return Crossing(Side::RIGHT, result);
        }

        throw std::runtime_error("Never get here!");
    }
#endif

    std::ostream &operator<<(std::ostream &os, const Cell &c) {
        os << "POLYGON ((";
        os << c.m_box.xmin << " " << c.m_box.ymin << ", ";
        os << c.m_box.xmax << " " << c.m_box.ymin << ", ";
        os << c.m_box.xmax << " " << c.m_box.ymax << ", ";
        os << c.m_box.xmin << " " << c.m_box.ymax << ", ";
        os << c.m_box.xmin << " " << c.m_box.ymin << ")";
        return os;
    }

}
