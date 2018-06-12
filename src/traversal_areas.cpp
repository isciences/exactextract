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

#include <iostream>
#include <limits>
#include <vector>

#include "area.h"
#include "box.h"
#include "coordinate.h"
#include "perimeter_distance.h"

namespace exactextract {

    struct CoordinateChain {
        double start;
        double stop;
        const std::vector<Coordinate> *coordinates;
        bool visited;

        CoordinateChain(double start, double stop, const std::vector<Coordinate> *coords) :
                start{start},
                stop{stop},
                coordinates{coords},
                visited{false} {}
    };

    static double
    exit_to_entry_perimeter_distance_ccw(const CoordinateChain &c1, const CoordinateChain &c2, double perimeter) {
        return perimeter_distance_ccw(c1.stop, c2.start, perimeter);
    }

    static CoordinateChain *next_chain(std::vector<CoordinateChain> &chains,
                                       const CoordinateChain *chain,
                                       const CoordinateChain *kill,
                                       double perimeter) {

        CoordinateChain *min = nullptr;
        double min_distance = std::numeric_limits<double>::max();

        for (CoordinateChain &candidate : chains) {
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

    double left_hand_area(const Box &box, const std::vector<const std::vector<Coordinate> *> &coord_lists) {
        std::vector<CoordinateChain> chains;

        for (const auto &coords : coord_lists) {
            double start = perimeter_distance(box, (*coords)[0]);
            double stop = perimeter_distance(box, (*coords)[coords->size() - 1]);

            chains.emplace_back(start, stop, coords);
        }

        double height{box.height()};
        double width{box.width()};
        double perimeter{box.perimeter()};

        // create coordinate lists for corners
        std::vector<Coordinate> bottom_left = {Coordinate(box.xmin, box.ymin)};
        std::vector<Coordinate> top_left = {Coordinate(box.xmin, box.ymax)};
        std::vector<Coordinate> top_right = {Coordinate(box.xmax, box.ymax)};
        std::vector<Coordinate> bottom_right = {Coordinate(box.xmax, box.ymin)};

        // Add chains for corners
        chains.emplace_back(0.0, 0.0, &bottom_left);
        chains.emplace_back(height, height, &top_left);
        chains.emplace_back(height + width, height + width, &top_right);
        chains.emplace_back(2 * height + width, 2 * height + width, &bottom_right);

        double sum{0.0};
        for (auto &chain_ref : chains) {
            if (chain_ref.visited || chain_ref.coordinates->size() == 1) {
                continue;
            }

            std::vector<Coordinate> coords;
            CoordinateChain *chain = std::addressof(chain_ref);
            CoordinateChain *first_chain = chain;
            do {
                chain->visited = true;
                coords.insert(coords.end(), chain->coordinates->cbegin(), chain->coordinates->cend());
                chain = next_chain(chains, chain, first_chain, perimeter);
            } while (chain != first_chain);

            coords.push_back(coords[0]);

            sum += area(coords);
        }

        return sum;
    }

}
