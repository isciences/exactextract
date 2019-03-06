// Copyright (c) 2018-2019 ISciences, LLC.
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

#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "CLI11.hpp"

#include "csv_writer.h"
#include "gdal_dataset_wrapper.h"
#include "gdal_raster_wrapper.h"
#include "gdal_writer.h"
#include "operation.h"
#include "processor.h"
#include "feature_sequential_processor.h"
#include "raster_sequential_processor.h"
#include "utils.h"
#include "version.h"

using exactextract::GDALDatasetWrapper;
using exactextract::GDALRasterWrapper;
using exactextract::Operation;

static bool stored_values_needed(const std::vector<std::string> & stats) {
    for (const auto& stat : stats) {
        if (stat == "mode" || stat == "majority" || stat == "minority" || stat == "variety")
            return true;
    }
    return false;
}

static std::unordered_map<std::string, GDALRasterWrapper> load_rasters(const std::vector<std::string> & descriptors);
static std::vector<Operation> prepare_operations(const std::vector<std::string> & descriptors,
        std::unordered_map<std::string, GDALRasterWrapper> & rasters);

int main(int argc, char** argv) {
    CLI::App app{"Zonal statistics using exactextract: build " + exactextract::version()};

    std::string poly_filename, field_name, output_filename, filter, strategy;
    std::vector<std::string> stats;
    std::vector<std::string> raster_descriptors;
    size_t max_cells_in_memory = 30;
    bool progress;
    app.add_option("-p", poly_filename, "polygon dataset")->required(true);
    app.add_option("-r", raster_descriptors, "raster dataset")->required(true);
    app.add_option("-f", field_name, "id from polygon dataset to retain in output")->required(true);
    app.add_option("-o", output_filename, "output filename")->required(true);
    app.add_option("-s", stats, "statistics")->required(true)->expected(-1);
    app.add_option("--filter", filter, "only process specified value of id")->required(false);
    app.add_option("--max-cells", max_cells_in_memory, "maximum number of raster cells to read in memory at once, in millions")->required(false)->default_val("30");
    app.add_option("--strategy", strategy, "processing strategy")->required(false)->default_val("feature-sequential");
    app.add_flag("--progress", progress);

    if (argc == 1) {
        std::cout << app.help();
        return 0;
    }
    CLI11_PARSE(app, argc, argv)
    max_cells_in_memory *= 1000000;

    std::unique_ptr<exactextract::Processor> proc;
    std::unique_ptr<exactextract::OutputWriter> writer;

    try {
        GDALAllRegister();

        auto rasters = load_rasters(raster_descriptors);

        // TODO have some way to select dataset within OGR dataset
        GDALDatasetWrapper shp{poly_filename, 0};

#if 0
        // Check grid compatibility
        if (!weights.empty()) {
            if (!values.grid().compatible_with(weights[0].grid())) {
                std::cerr << "Value and weighting rasters do not have compatible grids." << std::endl;
                std::cerr << "Value grid origin: (" << values.grid().xmin() << "," << values.grid().ymin() << ") resolution: (";
                std::cerr << values.grid().dx() << "," << values.grid().dy() << ")" << std::endl;
                std::cerr << "Weighting grid origin: (" << weights[0].grid().xmin() << "," << weights[0].grid().ymin() << ") resolution: (";
                std::cerr << weights[0].grid().dx() << "," << weights[0].grid().dy() << ")" << std::endl;
                return 1;
            }

            for (size_t i = 1; i < weights.size(); i++) {
                if (weights[i].grid() != weights[0].grid()) {
                    std::cerr << "All weighting rasters must have the same resolution and extent.";
                    return 1;
                }
            }
        }
#endif
        writer = std::make_unique<exactextract::GDALWriter>(output_filename, field_name);

        if (strategy == "feature-sequential") {
            proc = std::make_unique<exactextract::FeatureSequentialProcessor>(shp, *writer, field_name);
        } else if (strategy == "raster-sequential") {
            proc = std::make_unique<exactextract::RasterSequentialProcessor>(shp, *writer, field_name);
        } else {
            throw std::runtime_error("Unknown processing strategy: " + strategy);
        }

        for (const auto& op : prepare_operations(stats, rasters)) {
            proc->add_operation(op);
        }

        proc->set_max_cells_in_memory(max_cells_in_memory);

        proc->process();
        writer->finish();

        return 0;
    } catch (const std::exception & e) {
        std::cerr << "Error: " << e.what() << std::endl;

        return 1;
    } catch (...) {
        std::cerr << "Unknown error." << std::endl;

        return 1;
    }
}

static std::unordered_map<std::string, GDALRasterWrapper> load_rasters(const std::vector<std::string> & descriptors) {
    std::unordered_map<std::string, GDALRasterWrapper> rasters;

    for (const auto &descriptor : descriptors) {
        auto parsed = exactextract::parse_raster_descriptor(descriptor);

        auto name = std::get<0>(parsed);

        rasters.emplace(name, GDALRasterWrapper{std::get<1>(parsed), std::get<2>(parsed)});
        rasters.at(name).set_name(name);
    }

    return rasters;
}

static std::vector<Operation> prepare_operations(const std::vector<std::string> & descriptors,
        std::unordered_map<std::string, GDALRasterWrapper> & rasters) {
    std::vector<Operation> ops;

    for (const auto &descriptor : descriptors) {
        auto stat = exactextract::parse_stat_descriptor(descriptor);

        auto values_it = rasters.find(stat[0]);
        if (values_it == rasters.end()) {
            throw std::runtime_error("Unknown raster " + stat[0] + " in stat descriptor: " + descriptor);
        }

        GDALRasterWrapper* values = &(values_it->second);
        GDALRasterWrapper* weights;

        if (stat[1].empty()) {
            weights = nullptr;
        } else {
            auto weights_it = rasters.find(stat[1]);
            if (weights_it == rasters.end()) {
                throw std::runtime_error("Unknown raster " + stat[1] + " in stat descriptor: " + descriptor);
            }

            weights = &(weights_it->second);
        }

        ops.emplace_back(stat[2], values, weights);
    }
}
