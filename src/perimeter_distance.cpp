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

#include <stdexcept>

#include "perimeter_distance.h"

namespace exactextract {

    double perimeter_distance(double xmin, double ymin, double xmax, double ymax, double x, double y) {
        if (x == xmin) {
            // Left
            return y - ymin;
        }

        if (y == ymax) {
            // Top
            return (ymax - ymin) + x - xmin;
        }

        if (x == xmax) {
            // Right
            return (xmax - xmin) + (ymax - ymin) + ymax - y;
        }

        if (y == ymin) {
            // Bottom
            return (xmax - xmin) + 2 * (ymax - ymin) + (xmax - x);
        }

        throw std::runtime_error("Never get here");
    }

    double perimeter_distance(double xmin, double ymin, double xmax, double ymax, const Coordinate &c) {
        return perimeter_distance(xmin, ymin, xmax, ymax, c.x, c.y);
    }

    double perimeter_distance(const Box &b, const Coordinate &c) {
        return perimeter_distance(b.xmin, b.ymin, b.xmax, b.ymax, c.x, c.y);
    }

    double perimeter_distance_ccw(double measure1, double measure2, double perimeter) {
        if (measure2 <= measure1) {
            return measure1 - measure2;
        }
        return perimeter + measure1 - measure2;
    }

}
