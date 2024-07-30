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

#pragma once

#include <vector>

#include "coordinate.h"
#include "side.h"

namespace exactextract {

/**
 * @brief The Traversal class records the coordinates of a line that are
 *        within a grid cell, as well as the `Side` from which the line
 *        entered and exited the cell.
 */
class Traversal
{
  public:
    Traversal()
      : m_entry{ Side::NONE }
      , m_exit{ Side::NONE }
    {
    }

    bool is_closed_ring() const;

    bool empty() const;

    bool entered() const;

    bool exited() const;

    bool traversed() const;

    bool multiple_unique_coordinates() const;

    /// Begin a Traversal on the specified `Side`
    void enter(const Coordinate& c, Side s);

    /// Complete a Traversal on the specified `Side`
    void exit(const Coordinate& c, Side s);

    Side entry_side() const { return m_entry; }

    Side exit_side() const { return m_exit; }

    const Coordinate& last_coordinate() const;

    const Coordinate& exit_coordinate() const;

    void add(const Coordinate& c);

    void force_exit(Side s) { m_exit = s; }

    const std::vector<Coordinate>& coords() const { return m_coords; }

  private:
    std::vector<Coordinate> m_coords;
    Side m_entry;
    Side m_exit;
};

}
