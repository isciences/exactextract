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

#include <limits>
#include <vector>

#include "box.h"
#include "coordinate.h"
#include "geos_utils.h"
#include "measures.h"
#include "perimeter_distance.h"

namespace exactextract {

struct CoordinateChain
{
    double start; // perimeter distance value of the first coordinate
    double stop;  // perimeter distance value of the last coordinate
    const std::vector<Coordinate>* coordinates;
    bool visited;

    CoordinateChain(double p_start, double p_stop, const std::vector<Coordinate>* p_coords)
      : start{ p_start }
      , stop{ p_stop }
      , coordinates{ p_coords }
      , visited{ false }
    {
    }
};

static double
exit_to_entry_perimeter_distance_ccw(const CoordinateChain& c1, const CoordinateChain& c2, double perimeter)
{
    return perimeter_distance_ccw(c1.stop, c2.start, perimeter);
}

static CoordinateChain*
next_chain(std::vector<CoordinateChain>& chains,
           const CoordinateChain* chain,
           const CoordinateChain* kill,
           double perimeter)
{

    CoordinateChain* min = nullptr;
    double min_distance = std::numeric_limits<double>::max();

    for (CoordinateChain& candidate : chains) {
        if (candidate.visited && std::addressof(candidate) != kill) {
            continue;
        }

        double distance = exit_to_entry_perimeter_distance_ccw(*chain, candidate, perimeter);
        if (distance < min_distance) {
            min_distance = distance;
            min = std::addressof(candidate);
        }
    }

    return min;
}

template<typename T>
bool
has_multiple_unique_coordinates(const T& vec)
{
    for (std::size_t i = 1; i < vec.size(); i++) {
        if (vec[i] != vec[0]) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Identify counter-clockwise rings formed by the supplied coordinate vectors and the boundary of this box.
 *
 * @param context A GEOS context handle, needed for checking ring orientation
 * @param box Box to be included in rings
 * @param coord_lists A list of coordinate vectors, with each vector representating a traversal of `box` or a closed ring.
 * @param visitor Function be applied to each ring identifed. Because `coord_lists` may include both clockwise and
 *                counter-clockwise closed rings, `visitor` will be provided with the orientation of each ring as an
 *                argument.
 */
template<typename F>
void
visit_rings(GEOSContextHandle_t context, const Box& box, const std::vector<const std::vector<Coordinate>*>& coord_lists, F&& visitor)
{
    std::vector<CoordinateChain> chains;

    for (const auto& coords : coord_lists) {
        if (!has_multiple_unique_coordinates(*coords)) {
            continue;
        }

        if (coords->front() == coords->back() && has_multiple_unique_coordinates(*coords)) {
            // Closed ring. Check orientation.

            auto seq = to_coordseq(context, *coords);
            bool is_ccw = geos_is_ccw(context, seq.get());
            visitor(*coords, is_ccw);
        } else {
            double start = perimeter_distance(box, coords->front());
            double stop = perimeter_distance(box, coords->back());

            chains.emplace_back(start, stop, coords);
        }
    }

    double height{ box.height() };
    double width{ box.width() };
    double perimeter{ box.perimeter() };

    // create coordinate lists for corners
    std::vector<Coordinate> bottom_left = { Coordinate(box.xmin, box.ymin) };
    std::vector<Coordinate> top_left = { Coordinate(box.xmin, box.ymax) };
    std::vector<Coordinate> top_right = { Coordinate(box.xmax, box.ymax) };
    std::vector<Coordinate> bottom_right = { Coordinate(box.xmax, box.ymin) };

    // Add chains for corners
    chains.emplace_back(0.0, 0.0, &bottom_left);
    chains.emplace_back(height, height, &top_left);
    chains.emplace_back(height + width, height + width, &top_right);
    chains.emplace_back(2 * height + width, 2 * height + width, &bottom_right);

    for (auto& chain_ref : chains) {
        if (chain_ref.visited || chain_ref.coordinates->size() == 1) {
            continue;
        }

        std::vector<Coordinate> coords;
        CoordinateChain* chain = std::addressof(chain_ref);
        CoordinateChain* first_chain = chain;
        do {
            chain->visited = true;
            coords.insert(coords.end(), chain->coordinates->cbegin(), chain->coordinates->cend());
            chain = next_chain(chains, chain, first_chain, perimeter);
        } while (chain != first_chain);

        coords.push_back(coords[0]);

        if (has_multiple_unique_coordinates(coords)) {
            visitor(coords, true);
        }
    }
}

double
left_hand_area(const Box& box, const std::vector<const std::vector<Coordinate>*>& coord_lists)
{

    GEOSContextHandle_t context = GEOS_init_r();

    double ccw_sum = 0;
    double cw_sum = 0;
    bool found_a_ring = false;

    visit_rings(context, box, coord_lists, [&cw_sum, &ccw_sum, &found_a_ring](const std::vector<Coordinate>& coords, bool is_ccw) {
        found_a_ring = true;

        if (is_ccw) {
            ccw_sum += area(coords);
        } else {
            cw_sum += area(coords);
        }
    });

    GEOS_finish_r(context);

    if (!found_a_ring) {
        throw std::runtime_error("Cannot determine coverage fraction (it is either 0 or 100%)");
    }

    // If this box has only clockwise rings (holes) then the area
    // of those holes should be subtracted from the complete box area.
    if (ccw_sum == 0.0 && cw_sum > 0) {
        return box.area() - cw_sum;
    } else {
        return ccw_sum - cw_sum;
    }
}

geom_ptr_r
left_hand_rings(GEOSContextHandle_t context, const Box& box, const std::vector<const std::vector<Coordinate>*>& coord_lists)
{

    std::vector<GEOSGeometry*> shells;
    std::vector<GEOSGeometry*> holes;

    bool found_a_ring = false;

    visit_rings(context, box, coord_lists, [&context, &shells, &holes, &found_a_ring](const std::vector<Coordinate>& coords, bool is_ccw) {
        found_a_ring = true;

        // finding a collapsed ring is sufficient to determine whether the cell interior is inside our outside,
        // but we don't want to actually construct the ring.
        if (area(coords) == 0) {
            return;
        }

        auto seq = to_coordseq(context, coords);
        auto ring = GEOSGeom_createLinearRing_r(context, seq.release());

        if (is_ccw) {
            shells.push_back(ring);
        } else {
            holes.push_back(ring);
        }
    });

    if (!found_a_ring) {
        throw std::runtime_error("Cannot determine coverage fraction (it is either 0 or 100%)");
    }

    if (shells.empty() && !holes.empty()) {
        shells.push_back(geos_make_box_linearring(context, box).release());
    }

    if (holes.empty()) {
        std::vector<GEOSGeometry*> polygons;
        for (auto& shell : shells) {
            polygons.push_back(GEOSGeom_createPolygon_r(context, shell, nullptr, 0));
        }

        if (polygons.size() == 1) {
            return geos_ptr(context, polygons[0]);
        } else {
            return geos_ptr(context, GEOSGeom_createCollection_r(context, GEOS_MULTIPOLYGON, polygons.data(), polygons.size()));
        }
    } else if (shells.size() == 1) {
        return geos_ptr(context, GEOSGeom_createPolygon_r(context, shells.front(), holes.data(), holes.size()));
    } else {
#if HAVE_380
        std::vector<GEOSGeometry*> rings(shells);
        rings.insert(rings.end(), holes.begin(), holes.end());

        auto polygonized = geos_ptr(context, GEOSPolygonize_valid_r(context, rings.data(), rings.size()));

        for (auto& g : rings) {
            GEOSGeom_destroy_r(context, g);
        }

        return polygonized;
#else
        throw std::runtime_error("Cannot associate holes with shells with GEOS < 3.8");
#endif
    }
}

}
