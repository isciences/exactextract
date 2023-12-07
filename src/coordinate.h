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

#ifndef EXACTEXTRACT_COORDINATE_H
#define EXACTEXTRACT_COORDINATE_H

#include <cmath>
#include <iostream>

namespace exactextract {

struct Coordinate
{
    double x;
    double y;

    Coordinate() = default;

    Coordinate(double p_x, double p_y)
      : x{ p_x }
      , y{ p_y }
    {
    }

    double distance(const Coordinate& other) const
    {
        double dx = other.x - x;
        double dy = other.y - y;
        return std::sqrt(dx * dx + dy * dy);
    }

    bool equals(const Coordinate& other, double tol) const
    {
        return std::abs(other.x - x) < tol && std::abs(other.y - y) < tol;
    }

    bool operator==(const Coordinate& other) const
    {
        return x == other.x && y == other.y;
    }

    bool operator!=(const Coordinate& other) const
    {
        return !(*this == other);
    }
};

std::ostream&
operator<<(std::ostream& os, const Coordinate& c);

}

#endif