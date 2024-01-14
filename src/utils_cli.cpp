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

#include "utils_cli.h"
#include "gdal_raster_wrapper.h"

#include <filesystem>
#include <unordered_map>
#include <unordered_set>

#include <gdal.h>

namespace exactextract {
std::vector<std::unique_ptr<RasterSource>>
load_gdal_rasters(const std::vector<std::string>& descriptors)
{
    std::unordered_map<std::string, std::shared_ptr<GDALRaster>> rasters;
    std::vector<std::unique_ptr<RasterSource>> raster_sources;

    for (const auto& descriptor : descriptors) {
        auto [name, dsn, band] = exactextract::parse_raster_descriptor(descriptor);

        if (name.empty() && descriptors.size() > 1) {
            name = std::filesystem::path(dsn).replace_extension("").filename();
        }

        std::shared_ptr<GDALRaster>& raster = rasters[dsn];
        if (raster == nullptr) {
            raster = std::make_shared<GDALRaster>(dsn);
        }

        if (band == 0) {
            // Band was not specified
            const int bands = GDALGetRasterCount(raster->get());

            for (int i = 1; i <= bands; i++) {
                raster_sources.emplace_back(std::make_unique<GDALRasterWrapper>(raster, i));
                if (bands > 1) {
                    if (name.empty()) {
                        raster_sources.back()->set_name("band_" + std::to_string(i));
                    } else {
                        raster_sources.back()->set_name(name + "_band_" + std::to_string(i));
                    }
                } else {
                    raster_sources.back()->set_name(name);
                }
            }
        } else {
            // Band was specified
            raster_sources.emplace_back(std::make_unique<GDALRasterWrapper>(raster, band));
            raster_sources.back()->set_name(name);
        }
    }

    if (raster_sources.size() > 1) {
        std::unordered_set<std::string_view> names;
        for (const auto& src : raster_sources) {
            auto [_, inserted] = names.emplace(src->name());
            if (!inserted) {
                throw std::runtime_error("Duplicated raster name: " + src->name());
            }
        }
    }

    return raster_sources;
}
}