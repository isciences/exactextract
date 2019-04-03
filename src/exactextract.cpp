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

static GDALDatasetWrapper load_dataset(const std::string & descriptor, const std::string & field_name);
static std::unordered_map<std::string, GDALRasterWrapper> load_rasters(const std::vector<std::string> & descriptors);
static std::vector<Operation> prepare_operations(const std::vector<std::string> & descriptors,
        std::unordered_map<std::string, GDALRasterWrapper> & rasters);

int main(int argc, char** argv) {
    CLI::App app{"Zonal statistics using exactextract: build " + exactextract::version()};

    std::string poly_descriptor, field_name, output_filename, strategy;
    std::vector<std::string> stats;
    std::vector<std::string> raster_descriptors;
    size_t max_cells_in_memory = 30;
    bool progress;
    app.add_option("-p", poly_descriptor, "polygon dataset")->required(true);
    app.add_option("-r", raster_descriptors, "raster dataset")->required(true);
    app.add_option("-f", field_name, "id from polygon dataset to retain in output")->required(true);
    app.add_option("-o", output_filename, "output filename")->required(true);
    app.add_option("-s", stats, "statistics")->required(true)->expected(-1);
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

        GDALDatasetWrapper shp = load_dataset(poly_descriptor, field_name);

        auto gdal_writer = std::make_unique<exactextract::GDALWriter>(output_filename);
        gdal_writer->copy_id_field(shp);
        writer = std::move(gdal_writer);

        auto operations = prepare_operations(stats, rasters);

        if (strategy == "feature-sequential") {
            proc = std::make_unique<exactextract::FeatureSequentialProcessor>(shp, *writer, operations);
        } else if (strategy == "raster-sequential") {
            proc = std::make_unique<exactextract::RasterSequentialProcessor>(shp, *writer, operations);
        } else {
            throw std::runtime_error("Unknown processing strategy: " + strategy);
        }

        proc->set_max_cells_in_memory(max_cells_in_memory);
        proc->show_progress(progress);

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

static GDALDatasetWrapper load_dataset(const std::string & descriptor, const std::string & field_name) {
    auto parsed = exactextract::parse_dataset_descriptor(descriptor);

    return GDALDatasetWrapper{parsed.first, parsed.second, field_name};
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

        auto values_it = rasters.find(stat.values);
        if (values_it == rasters.end()) {
            throw std::runtime_error("Unknown raster " + stat.values + " in stat descriptor: " + descriptor);
        }

        GDALRasterWrapper* values = &(values_it->second);
        GDALRasterWrapper* weights;

        if (stat.weights.empty()) {
            weights = nullptr;
        } else {
            auto weights_it = rasters.find(stat.weights);
            if (weights_it == rasters.end()) {
                throw std::runtime_error("Unknown raster " + stat.weights + " in stat descriptor: " + descriptor);
            }

            weights = &(weights_it->second);
        }

        ops.emplace_back(stat.stat, stat.name, values, weights);
    }

    return ops;
}
