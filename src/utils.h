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
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
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

namespace string {

bool
read_bool(const std::string& value);
double
read_double(const std::string& value);
std::int64_t
read_int64(const std::string& value);
std::uint64_t
read_uint64(const std::string& value);

template<typename T>
T
read(const std::string& value)
{
    if constexpr (std::is_same_v<T, bool>) {
        return read_bool(value);
    } else if constexpr (std::is_integral_v<T>) {
        if (std::is_signed_v<T>) {
            std::int64_t parsed = read_int64(value);
            if (parsed > std::numeric_limits<T>::max() || parsed < std::numeric_limits<T>::min()) {
                throw std::runtime_error("Value out of range");
            } else {
                return static_cast<T>(parsed);
            }
        } else {
            for (auto& c : value) {
                if (c == '-') {
                    throw std::runtime_error("Value out of range");
                }
            }
            std::uint64_t parsed = read_uint64(value);
            if (parsed > std::numeric_limits<T>::max() || parsed < std::numeric_limits<T>::min()) {
                throw std::runtime_error("Value out of range");
            } else {
                return static_cast<T>(parsed);
            }
        }
    } else if constexpr (std::is_floating_point_v<T>) {
        double parsed = read_double(value);
        if (parsed > static_cast<double>(std::numeric_limits<T>::max()) || parsed < static_cast<double>(std::numeric_limits<T>::lowest())) {
            throw std::runtime_error("Value out of range");
        } else {
            return static_cast<T>(parsed);
        }
    } else {
        return value;
    }
}

}

}
