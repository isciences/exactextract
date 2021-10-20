// Copyright (c) 2018-2021 ISciences, LLC.
// All rights reserved.
//
// This software is licensed under the Apache License, Version 2.0 (the "License").
// You may not use this file except in compliance with the License. You may
// obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdexcept>

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

    bool Cell::take(const Coordinate& c, const Coordinate* prev_original) {
        Traversal &t = traversal_in_progress();

        if (t.empty()) {
            //std::cout << "Entering " << m_box << " from " << side(c) << " at " << c << std::endl;

            t.enter(c, side(c));
            return true;
        }

        if (location(c) != Cell::Location::OUTSIDE) {
            //std::cout << "Still in " << m_box << " with " << c << std::endl;

            t.add(c);
            return true;
        }

        // We need to calculate the coordinate of the cell exit point using only uninterpolated coordinates.
        // (The previous point in the traversal may be an interpolated coordinate.) If an interpolated coordinate
        // is used, it can cause an error in the relative position two traversals, inverting the fraction of
        // the cell that is considered covered. (See robustness regression test #7).
        Crossing x = prev_original ? m_box.crossing(*prev_original, c) : m_box.crossing(t.last_coordinate(), c);
        t.exit(x.coord(), x.side());

        //std::cout << "Leaving " << m_box << " from " << x.side() << " at " << x.coord();
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


}
