// Copyright (c) 2019 ISciences, LLC.
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

#include "utils.h"

#include <stdexcept>

namespace exactextract {

    std::tuple<std::string, std::string, int> parse_raster_descriptor(const std::string &descriptor) {
        if (descriptor.empty())
            throw std::runtime_error("Empty descriptor.");


        auto pos1 = descriptor.find(':');
        auto pos2 = descriptor.rfind('[');

        if (pos1 != std::string::npos && pos2 < pos1) {
            // Ignore [ character within name
            pos2 = std::string::npos;
        }

        std::string name;
        std::string fname;
        int band;

        if (pos1 != std::string::npos) {
            name = descriptor.substr(0, pos1);
        }

        if (pos2 == std::string::npos) {
            // No band was specified; set it to 1.
            fname = descriptor.substr(pos1 + 1);
            band = 1;
        } else {
            fname = descriptor.substr(pos1 + 1, pos2 - pos1 - 1);

            auto rest = descriptor.substr(pos2 + 1);
            band = std::stoi(rest);
        }

        if (pos1 == std::string::npos) {
            // No name was provided, so just use the filename
            name = fname;
        }

        if (fname.empty())
            throw std::runtime_error("Descriptor has no filename.");

        return std::make_tuple(name, fname, band);
    }

    std::array<std::string, 3> parse_stat_descriptor(const std::string & descriptor) {
        if (descriptor.empty()) {
            throw std::runtime_error("Invalid stat descriptor.");
        }

        auto pos1 = descriptor.find('(');

        if (pos1 == 0 || pos1 == std::string::npos || pos1 > (descriptor.size() - 3)) {
            throw std::runtime_error("Invalid stat descriptor.");
        }

        std::string stat = descriptor.substr(0, pos1);
        std::string values;
        std::string weights;

        auto pos2 = descriptor.find(',', pos1 + 1);

        if (pos2 == std::string::npos) {
            values = descriptor.substr(pos1 + 1, descriptor.size() - pos1 - 2);
            weights = "";
        } else {
            values = descriptor.substr(pos1 + 1, pos2 - pos1 - 1);
            weights = descriptor.substr(pos2 + 1, descriptor.size() - pos2 - 2);
        }

        return {{ values, weights, stat }};
    }

}
