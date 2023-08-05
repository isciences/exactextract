// Copyright (c) 2018-2023 ISciences, LLC.
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

#ifndef EXACTEXTRACT_TRAVERSAL_AREAS_H
#define EXACTEXTRACT_TRAVERSAL_AREAS_H

#include <vector>

#include "box.h"
#include "coordinate.h"

#include "geos_utils.h"

namespace exactextract {

    /**
     * @brief Return the total area of counter-clockwise closed rings formed by this box and the provided Coordinate sequences
     *
     * @param box boundary of the area to consider (Cell)
     * @param coord_lists vector of Coordinate vectors representing points that traverse `box`. Either the first and
     *                    last coordinate of each vector must lie on the boundary of `box`, or the coordinates
     *                    must form a closed ring that does not intersect the boundary of `box.` Clockwise-oriented
     *                    closed rings will be considered holes.
     * @return total area
     */
    double left_hand_area(const Box &box, const std::vector<const std::vector<Coordinate> *> &coord_lists);

    /**
     * @brief Return an areal geometry representing the closed rings formed by this box and the provided Coordinate sequences
     *
     * @param context GEOS context handle
     * @param box boundary of the area to consider (Cell)
     * @param coord_lists vector of Coordinate vectors representing points that traverse `box`. Either the first and
     *                    last coordinate of each vector must lie on the boundary of `box`, or the coordinates
     *                    must form a closed ring that does not intersect the boundary of `box.` Clockwise-oriented
     *                    closed rings will be considered holes.
     * @return a Polygon or MultiPolygon geometry
     */
    geom_ptr_r left_hand_rings(GEOSContextHandle_t context, const Box &box, const std::vector<const std::vector<Coordinate> *> &coord_lists);

}

#endif
