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

#ifndef EXACTEXTRACT_CELL_CROSSING_H
#define EXACTEXTRACT_CELL_CROSSING_H

#include "coordinate.h"
#include "side.h"

namespace exactextract {

    class Crossing {
    public:
        Crossing(Side s, double x, double y) : m_side{s}, m_coord{x, y} {}

        Crossing(Side s, const Coordinate &c) : m_side{s}, m_coord{c} {}

        const Side &side() const {
            return m_side;
        }

        const Coordinate &coord() const {
            return m_coord;
        }

    private:
        Side m_side;
        Coordinate m_coord;

    };

}

#endif