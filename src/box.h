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

#ifndef EXACTEXTRACT_BOX_H
#define EXACTEXTRACT_BOX_H

#include "coordinate.h"
#include "crossing.h"
#include "side.h"

#include <limits>

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

        static Box maximum_finite() {
            return {
                std::numeric_limits<double>::lowest(),
                std::numeric_limits<double>::lowest(),
                std::numeric_limits<double>::max(),
                std::numeric_limits<double>::max()
            };
        }

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

        bool operator==(const Box& other) const {
            return xmin == other.xmin && xmax == other.xmax && ymin == other.ymin && ymax == other.ymax;
        }

    };

}

#endif
