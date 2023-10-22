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

#include <regex>
#include <sstream>
#include <stdexcept>

namespace exactextract {

    std::pair<std::string, std::string> parse_dataset_descriptor(const std::string & descriptor) {
        if (descriptor.empty())
            throw std::runtime_error("Empty descriptor.");

        auto pos = descriptor.rfind('[');

        if (pos == std::string::npos) {
            return std::make_pair(descriptor, "0");
        }

        return std::make_pair(descriptor.substr(0, pos),
                              descriptor.substr(pos + 1, descriptor.length() - pos - 2));
    }

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

     StatDescriptor parse_stat_descriptor(const std::string & descriptor) {
         if (descriptor.empty()) {
             throw std::runtime_error("Invalid stat descriptor.");
         }

         StatDescriptor ret;

         // Parse optional name for stat, specified as NAME=stat(...)
         const std::regex re_result_name("^(\\w+)=");
         std::smatch result_name_match;
         if (std::regex_search(descriptor, result_name_match, re_result_name)) {
             ret.name = result_name_match[1].str();
         }

         // Parse name of value raster
         const std::regex re_func_name("=?(\\w+)\\(");
         std::smatch func_name_match;
         if (std::regex_search(descriptor, func_name_match, re_func_name)) {
             ret.stat = func_name_match[1].str();
         } else {
             throw std::runtime_error("Invalid stat descriptor.");
         }

         // Parse name of weight raster
         const std::regex re_args(R"(\(([,\w]+)+\)$)");
         std::smatch arg_names_match;
         if (std::regex_search(descriptor, arg_names_match, re_args)) {
             auto args = arg_names_match[1].str();

             auto pos = args.find(',');
             if (pos == std::string::npos) {
                 ret.values = std::move(args);
             } else {
                 ret.values = args.substr(0, pos);
                 ret.weights = args.substr(pos + 1);
             }
         } else {
             throw std::runtime_error("Invalid stat descriptor.");
         }

         // Construct a name if one was not specified
         if (ret.name.empty()) {
             std::ostringstream ss;
             ss << ret.values << '_' << ret.stat;

             if (!ret.weights.empty()) {
                 ss << '_' << ret.weights;
             }

             ret.name = ss.str();
         }

        return ret;
    }

}
