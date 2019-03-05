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

#ifndef EXACTEXTRACT_UTILS_H
#define EXACTEXTRACT_UTILS_H

#include <array>
#include <string>
#include <tuple>

namespace exactextract {
    std::tuple<std::string, std::string, int> parse_raster_descriptor(const std::string &descriptor);
    std::array<std::string, 3> parse_stat_descriptor(const std::string & descriptor);
}


#endif //EXACTEXTRACT_UTILS_H
