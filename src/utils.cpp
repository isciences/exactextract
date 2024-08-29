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

#include "utils.h"
#include "operation.h"
#include "raster_source.h"

#include <regex>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace exactextract {

// https://stackoverflow.com/a/46931770
std::vector<std::string>
split(const std::string& s, char delim)
{
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;

    while (getline(ss, item, delim)) {
        result.push_back(item);
    }

    return result;
}

std::pair<std::string, std::string>
parse_dataset_descriptor(const std::string& descriptor)
{
    if (descriptor.empty())
        throw std::runtime_error("Empty descriptor.");

    auto pos = descriptor.rfind('[');

    if (pos == std::string::npos) {
        return std::make_pair(descriptor, "0");
    }

    return std::make_pair(descriptor.substr(0, pos),
                          descriptor.substr(pos + 1, descriptor.length() - pos - 2));
}

std::tuple<std::string, std::string, int>
parse_raster_descriptor(const std::string& descriptor)
{
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
        // No band was specified; set it to 0.
        fname = descriptor.substr(pos1 + 1);
        band = 0;
    } else {
        fname = descriptor.substr(pos1 + 1, pos2 - pos1 - 1);

        auto rest = descriptor.substr(pos2 + 1);
        band = std::stoi(rest);
    }

    if (fname.empty())
        throw std::runtime_error("Descriptor has no filename.");

    return std::make_tuple(name, fname, band);
}

StatDescriptor
parse_stat_descriptor(const std::string& p_descriptor)
{
    if (p_descriptor.empty()) {
        throw std::runtime_error("Invalid stat descriptor. Descriptor is empty.");
    }

    std::string descriptor = p_descriptor;

    StatDescriptor ret;

    // Parse optional name for stat, specified as NAME=stat(...)
    const std::regex re_result_name("^(\\w+)=");
    std::smatch result_name_match;
    if (std::regex_search(descriptor, result_name_match, re_result_name)) {
        ret.name = result_name_match[1].str();
        descriptor.erase(0, result_name_match.length());
    }

    // Parse name of stat
    const std::regex re_func_name("=?(\\w+)\\(?");
    std::smatch func_name_match;
    if (std::regex_search(descriptor, func_name_match, re_func_name)) {
        ret.stat = func_name_match[1].str();
        descriptor.erase(0, func_name_match.length(1));
    } else {
        throw std::runtime_error("Invalid stat descriptor. No stat name found.");
    }

    // Parse stat arguments
    const std::regex re_args(R"(\(([ ,.=\w-]+)+\)$)");
    std::smatch args_match;
    if (std::regex_search(descriptor, args_match, re_args)) {
        auto args = split(args_match[1].str(), ',');
        descriptor.erase(0, args_match.length());

        for (std::size_t i = 0; i < args.size(); i++) {
            ltrim(args[i]);

            if (args[i].empty()) {
                throw std::runtime_error("Invalid stat descriptor. Empty argument.");
            }

            auto kv = split(args[i], '=');
            if (kv.size() == 1) {
                if (!ret.args.empty()) {
                    throw std::runtime_error("Invalid stat descriptor. Raster name provided after keyword arguments.");
                }

                if (i == 0) {
                    ret.values = std::move(args[i]);
                } else if (i == 1) {
                    ret.weights = std::move(args[i]);
                } else {
                    throw std::runtime_error("Invalid stat descriptor. Expected keyword argument.");
                }
            } else if (kv.size() == 2) {
                const auto& arg_name = kv.front();
                const auto& arg_value = kv.back();

                bool inserted = ret.args.try_emplace(arg_name, arg_value).second;
                if (!inserted) {
                    throw std::runtime_error("Invalid stat descriptor. Argument " + arg_name + " + specified multiple times.");
                }
            } else {
                throw std::runtime_error("Invalid stat descriptor. Malformed keyword argument: " + args[i]);
                ;
            }
        }
    }

    if (!descriptor.empty()) {
        throw std::runtime_error("Invalid stat descriptor. Failed to parse: " + descriptor);
    }

    return ret;
}

std::string
StatDescriptor::as_string() const
{
    std::stringstream desc;
    if (!name.empty()) {
        desc << name << "=";
    }
    desc << stat;

    if (values.empty() && weights.empty() && args.empty()) {
        return desc.str();
    }

    desc << "(";

    bool comma = false;

    if (!values.empty()) {
        desc << values;
        comma = true;
    }
    if (!weights.empty()) {
        if (comma) {
            desc << ",";
        }
        desc << weights;
        comma = true;
    }
    for (const auto& [k, v] : args) {
        if (comma) {
            desc << ",";
        }
        desc << k << "=" << v;
        comma = true;
    }
    desc << ")";
    return desc.str();
}

static std::string
make_name(const RasterSource* v, const RasterSource* w, const std::string& stat, bool full_names)
{
    if (!full_names) {
        return stat;
    }

    if (starts_with(stat, "weighted")) {
        if (w == nullptr) {
            throw std::runtime_error("No weights specified for stat: " + stat);
        }

        return v->name() + "_" + w->name() + "_" + stat;
    }

    return v->name() + "_" + stat;
}

static void
prepare_operations_implicit(
  std::vector<std::unique_ptr<Operation>>& ops,
  const StatDescriptor& sd,
  const RasterSourceVect& values,
  const RasterSourceVect& weights)
{
    std::size_t nvalues = values.size();
    std::size_t nweights = weights.size();
    if (!starts_with(sd.stat, "weighted")) {
        // Avoid looping over weights and generating duplicate Operations
        // for an unweighted stat
        nweights = std::min(nweights, std::size_t{ 1 });
    }

    const bool full_names = values.size() > 1 || weights.size() > 1;

    if (nvalues > 1 && nweights > 1 && nvalues != nweights) {
        throw std::runtime_error("Value and weight rasters must have a single band or the same number of bands.");
    }

    std::size_t nops = std::max(nvalues, nweights);
    for (std::size_t i = 0; i < nops; i++) {
        RasterSource* v = &*values[i % values.size()];
        RasterSource* w = weights.empty() ? nullptr : &*weights[i % weights.size()];

        ops.push_back(Operation::create(
          sd.stat,
          sd.name.empty() ? make_name(v, w, sd.stat, full_names) : sd.name,
          v,
          w,
          sd.args));
    }
}

void
prepare_operations_explicit(
  std::vector<std::unique_ptr<Operation>>& ops,
  const StatDescriptor& stat,
  const RasterSourceVect& raster_sources,
  const RasterSourceVect& weight_sources)
{
    std::unordered_map<std::string, RasterSource*> source_map;
    std::unordered_map<std::string, RasterSource*> weights_map;

    for (const auto& rast : raster_sources) {
        source_map[rast->name()] = rast;
        weights_map[rast->name()] = rast;
    }

    for (const auto& rast : weight_sources) {
        weights_map[rast->name()] = rast;
    }

    auto values_it = source_map.find(stat.values);
    if (values_it == source_map.end()) {
        throw std::runtime_error("Unknown raster " + stat.values + " in stat " + stat.stat);
    }

    RasterSource* values = values_it->second;
    RasterSource* weights = nullptr;

    if (!stat.weights.empty()) {
        auto weights_it = weights_map.find(stat.weights);
        if (weights_it == weights_map.end()) {
            weights_it = source_map.find(stat.weights);

            if (weights_it == weights_map.end()) {
                throw std::runtime_error("Unknown raster " + stat.weights + " in stat " + stat.stat);
            }
        }

        weights = weights_it->second;
    }

    ops.emplace_back(Operation::create(stat.stat, stat.name.empty() ? values->name() + "_" + stat.stat : stat.name, values, weights, stat.args));
}

std::vector<std::unique_ptr<Operation>>
prepare_operations(const std::vector<std::string>& descriptors,
                   const std::vector<std::unique_ptr<RasterSource>>& p_rasters,
                   const std::vector<std::unique_ptr<RasterSource>>& p_weights)
{
    std::vector<RasterSource*> rasters(p_rasters.size());
    std::vector<RasterSource*> weights(p_weights.size());

    for (std::size_t i = 0; i < p_rasters.size(); i++) {
        rasters[i] = p_rasters[i].get();
    }
    for (std::size_t i = 0; i < p_weights.size(); i++) {
        weights[i] = p_weights[i].get();
    }

    return prepare_operations(descriptors, rasters, weights);
}

static void
check_unique_names(const std::vector<std::unique_ptr<Operation>>& ops)
{
    std::unordered_set<std::string> op_names;
    for (const auto& op : ops) {
        bool inserted = op_names.insert(op->name).second;
        if (!inserted) {
            throw std::runtime_error("Operation name is not unique: " + op->name);
        }
    }
}

std::vector<std::unique_ptr<Operation>>
prepare_operations(
  const std::vector<std::string>& descriptors,
  const RasterSourceVect& rasters,
  const RasterSourceVect& weights)
{
    if (!descriptors.empty() && rasters.empty()) {
        throw std::runtime_error("no rasters provided to prepare_operations");
    }

    std::vector<std::unique_ptr<Operation>> ops;

    std::vector<StatDescriptor> parsed_descriptors;
    for (const auto& descriptor : descriptors) {
        auto parsed = parse_stat_descriptor(descriptor);
        if (parsed.values.empty() && parsed.weights.empty()) {
            prepare_operations_implicit(ops, parsed, rasters, weights);
        } else {
            prepare_operations_explicit(ops, parsed, rasters, weights);
        }
    }

    check_unique_names(ops);

    return ops;
}

namespace string {

bool
read_bool(const std::string& value)
{
    std::string value_lower = value;
    for (auto& c : value_lower) {
        c = std::tolower(c);
    }

    if (value_lower == "yes" || value_lower == "true") {
        return true;
    }
    if (value_lower == "no" || value_lower == "false") {
        return false;
    }

    throw std::runtime_error("Failed to parse value: " + value);
}

std::int64_t
read_int64(const std::string& value)
{
    char* end = nullptr;
    double d = std::strtol(value.data(), &end, 10);
    if (end == value.data() + value.size()) {
        return d;
    }

    throw std::runtime_error("Failed to parse value: " + value);
}

std::uint64_t
read_uint64(const std::string& value)
{
    char* end = nullptr;
    double d = std::strtoul(value.data(), &end, 10);
    if (end == value.data() + value.size()) {
        return d;
    }

    throw std::runtime_error("Failed to parse value: " + value);
}

double
read_double(const std::string& value)
{
    char* end = nullptr;
    double d = std::strtod(value.data(), &end);
    if (end == value.data() + value.size()) {
        return d;
    }

    throw std::runtime_error("Failed to parse value: " + value);
}

}

}
