// Copyright (c) 2018-2020 ISciences, LLC.
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

#include <vector>

#include "coordinate.h"

namespace exactextract {

enum class AreaMethods
{
    NONE,
    CARTESIAN,
    SPHERICAL
};

enum class AreaUnit
{
    M2,
    KM2
};

double
area_signed(const std::vector<Coordinate>& ring);

double
area(const std::vector<Coordinate>& ring);

double
length(const std::vector<Coordinate>& coords);

}
