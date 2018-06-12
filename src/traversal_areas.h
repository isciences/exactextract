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

#ifndef TRAVERSAL_AREAS_H
#define TRAVERSAL_AREAS_H

#include <vector>

#include "box.h"
#include "coordinate.h"

namespace exactextract {

    double left_hand_area(const Box &box, const std::vector<const std::vector<Coordinate> *> &coord_lists);

}

#endif