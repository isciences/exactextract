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

#ifndef EXACTEXTRACT_PERIMETERDISTANCE_H
#define EXACTEXTRACT_PERIMETERDISTANCE_H

#include "box.h"
#include "coordinate.h"

namespace exactextract {

double
perimeter_distance(double xmin, double ymin, double xmax, double ymax, double x, double y);

double
perimeter_distance(double xmin, double ymin, double xmax, double ymax, const Coordinate& c);

double
perimeter_distance(const Box& b, const Coordinate& c);

double
perimeter_distance_ccw(double measure1, double measure2, double perimeter);

}

#endif