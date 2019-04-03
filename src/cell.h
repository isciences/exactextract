// Copyright (c) 2018 ISciences, LLC.
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

#ifndef EXACTEXTRACT_CELL_H
#define EXACTEXTRACT_CELL_H

#include <memory>

#include "box.h"
#include "crossing.h"
#include "coordinate.h"
#include "side.h"
#include "traversal.h"

namespace exactextract {

    class Cell {

    public:

        Cell(double xmin, double ymin, double xmax, double ymax) :
                m_box{xmin, ymin, xmax, ymax} {}

        explicit Cell(const Box & b) : m_box{b} {}

        void force_exit();

        double width() const;

        double height() const;

        double area() const;

        double covered_fraction() const;

        Traversal &last_traversal();

        const Box &box() const { return m_box; }

        bool take(const Coordinate &c);

    private:
        enum class Location {
            INSIDE, OUTSIDE, BOUNDARY
        };

        Box m_box;

        std::vector<Traversal> m_traversals;

        Side side(const Coordinate &c) const;

        Location location(const Coordinate &c) const;

        Traversal &traversal_in_progress();
    };

}

#endif