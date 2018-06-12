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

#include "area.h"

#include <cmath>
#include <cstddef>

namespace exactextract {

    double area_signed(const std::vector<Coordinate> &ring) {
        if (ring.size() < 3) {
            return 0;
        }

        double sum{0};

        double x0{ring[0].x};
        for (size_t i = 1; i < ring.size() - 1; i++) {
            double x = ring[i].x - x0;
            double y1 = ring[i + 1].y;
            double y2 = ring[i - 1].y;
            sum += x * (y2 - y1);
        }

        return sum / 2.0;
    }

    double area(const std::vector<Coordinate> &ring) {
        return std::abs(area_signed(ring));
    }

}
