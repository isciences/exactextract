// Copyright (c) 2019-2024 ISciences, LLC.
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

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace exactextract {

class Operation;
class RasterSource;

struct StatDescriptor
{
    std::string name;
    std::string values;
    std::string weights;
    std::string stat;
    std::map<std::string, std::string> args;
};

std::pair<std::string, std::string>
parse_dataset_descriptor(const std::string& descriptor);
std::tuple<std::string, std::string, int>
parse_raster_descriptor(const std::string& descriptor);
StatDescriptor
parse_stat_descriptor(const std::string& descriptor);

using RasterSourceVect = std::vector<RasterSource*>;

std::vector<std::unique_ptr<Operation>>
prepare_operations(const std::vector<std::string>& descriptors,
                   const RasterSourceVect& rasters,
                   const RasterSourceVect& weights);

std::vector<std::unique_ptr<Operation>>
prepare_operations(const std::vector<std::string>& descriptors,
                   const std::vector<std::unique_ptr<RasterSource>>& rasters,
                   const std::vector<std::unique_ptr<RasterSource>>& weights);

// https://stackoverflow.com/a/2072890
inline bool
ends_with(std::string const& value, std::string const& suffix)
{
    if (suffix.size() > value.size())
        return false;
    return std::equal(suffix.rbegin(), suffix.rend(), value.rbegin());
}

inline bool
starts_with(std::string const& value, std::string const& prefix)
{
    if (prefix.size() > value.size())
        return false;
    return std::equal(prefix.cbegin(), prefix.cend(), value.cbegin());
}

std::vector<std::string>
split(const std::string& value, char delim);

// https://stackoverflow.com/a/217605/2171894
inline void
ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
}

}
