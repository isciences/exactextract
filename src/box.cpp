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

#include <cmath>

#include "box.h"

namespace exactextract {

    Side Box::side(const Coordinate &c) const {
        if (c.x == xmin) {
            return Side::LEFT;
        } else if (c.x == xmax) {
            return Side::RIGHT;
        } else if (c.y == ymin) {
            return Side::BOTTOM;
        } else if (c.y == ymax) {
            return Side::TOP;
        }

        return Side::NONE;
    }

    Crossing Box::crossing(const Coordinate &c1, const Coordinate &c2) const {
        // vertical line
        if (c1.x == c2.x) {
            if (c2.y >= ymax) {
                return Crossing{Side::TOP, c1.x, ymax};
            } else if (c2.y <= ymin) {
                return Crossing{Side::BOTTOM, c1.x, ymin};
            } else {
                throw std::runtime_error("Never get here.");
            }
        }

        // horizontal line
        if (c1.y == c2.y) {
            if (c2.x >= xmax) {
                return Crossing{Side::RIGHT, xmax, c1.y};
            } else if (c2.x <= xmin) {
                return Crossing{Side::LEFT, xmin, c1.y};
            } else {
                throw std::runtime_error("Never get here");
            }
        }

        double m = std::abs((c2.y - c1.y) / (c2.x - c1.x));

        bool up = c2.y > c1.y;
        bool right = c2.x > c1.x;

        if (up) {
            if (right) {
                // 1st quadrant
                double y2 = c1.y + m * (xmax - c1.x);

                if (y2 < ymax) {
                    return Crossing{Side::RIGHT, xmax, y2};
                } else {
                    double x2 = c1.x + (ymax - c1.y) / m;
                    return Crossing{Side::TOP, x2, ymax};
                }
            } else {
                // 2nd quadrant
                double y2 = c1.y + m * (c1.x - xmin);

                if (y2 < ymax) {
                    return Crossing{Side::LEFT, xmin, y2};
                } else {
                    double x2 = c1.x - (ymax - c1.y) / m;
                    return Crossing{Side::TOP, x2, ymax};
                }
            }
        } else {
            if (right) {
                // 4th quadrant
                double y2 = c1.y - m * (xmax - c1.x);

                if (y2 > ymin) {
                    return Crossing{Side::RIGHT, xmax, y2};
                } else {
                    double x2 = c1.x + (c1.y - ymin) / m;
                    return Crossing{Side::BOTTOM, x2, ymin};
                }
            } else {
                // 3rd quadrant
                double y2 = c1.y - m * (c1.x - xmin);

                if (y2 > ymin) {
                    return Crossing{Side::LEFT, xmin, y2};
                } else {
                    double x2 = c1.x - (c1.y - ymin) / m;
                    return Crossing{Side::BOTTOM, x2, ymin};
                }
            }
        }

    }

    bool Box::contains(const Coordinate &c) const {
        return c.x >= xmin && c.x <= xmax && c.y >= ymin && c.y <= ymax;
    }

    bool Box::strictly_contains(const Coordinate &c) const {
        return c.x > xmin && c.x < xmax && c.y > ymin && c.y < ymax;
    }

}
