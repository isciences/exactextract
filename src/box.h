// Copyright (c) 2018-2019 ISciences, LLC.
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

#pragma once

#include "coordinate.h"
#include "crossing.h"
#include "side.h"

#include <limits>

namespace exactextract {

struct Box
{
    double xmin;
    double ymin;
    double xmax;
    double ymax;

    Box(double p_xmin, double p_ymin, double p_xmax, double p_ymax)
      : xmin{ p_xmin }
      , ymin{ p_ymin }
      , xmax{ p_xmax }
      , ymax{ p_ymax }
    {
    }

    static Box maximum_finite()
    {
        return {
            std::numeric_limits<double>::lowest(),
            std::numeric_limits<double>::lowest(),
            std::numeric_limits<double>::max(),
            std::numeric_limits<double>::max()
        };
    }

    static Box make_empty()
    {
        return { 0, 0, 0, 0 };
    }

    double width() const
    {
        return xmax - xmin;
    }

    double height() const
    {
        return ymax - ymin;
    }

    double area() const
    {
        return width() * height();
    }

    double perimeter() const
    {
        return 2 * width() + 2 * height();
    }

    bool intersects(const Box& other) const
    {
        if (other.ymin > ymax)
            return false;
        if (other.ymax < ymin)
            return false;
        if (other.xmin > xmax)
            return false;
        if (other.xmax < xmin)
            return false;

        return true;
    }

    Box intersection(const Box& other) const
    {
        return {
            std::max(xmin, other.xmin),
            std::max(ymin, other.ymin),
            std::min(xmax, other.xmax),
            std::min(ymax, other.ymax)
        };
    }

    Box translate(double dx, double dy) const
    {
        return {
            xmin + dx,
            ymin + dy,
            xmax + dx,
            ymax + dy
        };
    }

    Coordinate upper_left() const
    {
        return Coordinate{ xmin, ymax };
    }

    Coordinate upper_right() const
    {
        return Coordinate{ xmax, ymax };
    }

    Coordinate lower_left() const
    {
        return Coordinate{ xmin, ymin };
    }

    Coordinate lower_right() const
    {
        return Coordinate{ xmax, ymin };
    }

    Side side(const Coordinate& c) const;

    Crossing crossing(const Coordinate& c1, const Coordinate& c2) const;

    bool empty() const
    {
        return xmin >= xmax || ymin >= ymax;
    }

    Box expand_to_include(const Box& other) const
    {
        if (empty()) {
            return other;
        }

        if (other.empty()) {
            return *this;
        }

        return { std::min(xmin, other.xmin),
                 std::min(ymin, other.ymin),
                 std::max(xmax, other.xmax),
                 std::max(ymax, other.ymax) };
    }

    bool contains(const Box& b) const;

    bool contains(const Coordinate& c) const;

    bool strictly_contains(const Coordinate& c) const;

    bool operator==(const Box& other) const
    {
        return xmin == other.xmin && xmax == other.xmax && ymin == other.ymin && ymax == other.ymax;
    }

    bool operator!=(const Box& other) const
    {
        return !(*this == other);
    }

    friend std::ostream& operator<<(std::ostream& os, const Box& c);
};

std::ostream&
operator<<(std::ostream& os, const Box& c);

}
