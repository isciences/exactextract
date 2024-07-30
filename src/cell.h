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

#ifndef EXACTEXTRACT_CELL_H
#define EXACTEXTRACT_CELL_H

#include "box.h"
#include "coordinate.h"
#include "geos_utils.h"
#include "side.h"
#include "traversal.h"

namespace exactextract {

/**
 * @brief The Cell class stores information about the spatial extent of a `Grid` cell and
 *        any cases where a line crosses that cell (recorded in a `Traversal`).
 */
class Cell
{

  public:
    Cell(double xmin, double ymin, double xmax, double ymax)
      : m_box{ xmin, ymin, xmax, ymax }
    {
    }

    explicit Cell(const Box& b)
      : m_box{ b }
    {
    }

    const Box& box() const { return m_box; }

    double width() const;

    double height() const;

    double area() const;

    /// Force the last Coordinate processed (via `take`) to be considered as an
    /// exit point, provided that it lies on the boundary of this Cell.
    void force_exit();

    /// Returns whether the cell can be determined to be wholly or partially
    /// covered by a polygon.
    bool determined() const;

    /// Return the total length of a linear geometry within this Cell
    double traversal_length() const;

    /// Return the fraction of this Cell that is covered by a polygon
    double covered_fraction() const;

    /// Return a newly constructed geometry representing the portion of this Cell
    /// that is covered by a polygon
    geom_ptr_r covered_polygons(GEOSContextHandle_t context) const;

    /// Return the last (most recent) traversal to which a coordinate has been
    /// added. The traversal may or may not be completed.
    Traversal& last_traversal();

    /**
     * Attempt to take a coordinate and add it to a Traversal in progress, or start a new Traversal
     *
     * @param c             Coordinate to process
     * @param prev_original The last *uninterpolated* coordinate preceding `c` in the
     *                      boundary being processed
     *
     * @return `true` if the Coordinate was inside this cell, `false` otherwise
     */
    bool take(const Coordinate& c, const Coordinate* prev_original = nullptr);

  private:
    std::vector<const std::vector<Coordinate>*> get_coord_lists() const;

    enum class Location
    {
        INSIDE,
        OUTSIDE,
        BOUNDARY
    };

    Box m_box;

    std::vector<Traversal> m_traversals;

    Side side(const Coordinate& c) const;

    Location location(const Coordinate& c) const;

    /// If no Traversals have been started or the most recent Traversal has been completed,
    /// return a new Traversal. Otherwise, return the most recent Traversal.
    Traversal& traversal_in_progress();
};

}

#endif
