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

#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>

#include <geos_c.h>
#include <gdal.h>

#include "CLI11.hpp"

#include "box.h"
#include "csv_utils.h"
#include "grid.h"
#include "gdal_dataset_wrapper.h"
#include "gdal_raster_wrapper.h"
#include "geos_utils.h"
#include "raster.h"
#include "raster_stats.h"
#include "raster_cell_intersection.h"

using exactextract::Box;
using exactextract::GDALDatasetWrapper;
using exactextract::GDALRasterWrapper;
using exactextract::Grid;
using exactextract::Raster;
using exactextract::RasterStats;
using exactextract::RasterView;
using exactextract::bounded_extent;
using exactextract::geos_ptr;
using exactextract::subdivide;
using exactextract::write_csv_header;
using exactextract::write_nas_to_csv;
using exactextract::write_stat_to_csv;
using exactextract::write_stats_to_csv;

static bool stored_values_needed(const std::vector<std::string> & stats) {
    for (const auto& stat : stats) {
        if (stat == "mode" || stat == "majority" || stat == "minority" || stat == "variety")
            return true;
    }
    return false;
}

int main(int argc, char** argv) {
    CLI::App app{"Zonal statistics using exactextract"};

    std::string poly_filename, rast_filename, field_name, output_filename, filter;
    std::vector<std::string> stats;
    std::vector<std::string> weights_filenames;
    size_t max_cells_in_memory = 30;
    bool progress;
    app.add_option("-p", poly_filename, "polygon dataset")->required(true);
    app.add_option("-r", rast_filename, "raster values dataset")->required(true);
    app.add_option("-w", weights_filenames, "optional raster weights dataset(s)")->required(false);
    app.add_option("-f", field_name, "id from polygon dataset to retain in output")->required(true);
    app.add_option("-o", output_filename, "output filename")->required(true);
    app.add_option("-s", stats, "statistics")->required(true)->expected(-1);
    app.add_option("--filter", filter, "only process specified value of id")->required(false);
    app.add_option("--max-cells", max_cells_in_memory, "maximum number of raster cells to read in memory at once, in millions")->required(false)->default_val("30");
    app.add_flag("--progress", progress);

    if (argc == 1) {
        std::cout << app.help();
        return 0;
    }
    CLI11_PARSE(app, argc, argv);
    bool use_weights = !weights_filenames.empty();
    max_cells_in_memory *= 1000000;

    GDALAllRegister();
    GEOSContextHandle_t geos_context = initGEOS_r(nullptr, nullptr);

    // Open GDAL datasets for out inputs
    GDALRasterWrapper values{rast_filename, 1};

    std::vector<GDALRasterWrapper> weights;

    for (const auto& filename : weights_filenames) {
        weights.emplace_back(filename, 1);
    }

    GDALDatasetWrapper shp{poly_filename, 0};

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

    std::ofstream csvout;
    csvout.open(output_filename);
    write_csv_header(field_name, stats, weights.size(), csvout);

    bool store_values = stored_values_needed(stats);
    std::vector<std::string> failures;
    while (shp.next()) {
        std::string name{shp.feature_field(field_name)};

        if (filter.length() == 0 || name == filter) {
            auto geom = geos_ptr(geos_context, shp.feature_geometry(geos_context));

            if (progress) std::cout << "Processing " << name << std::flush;

            try {
                Box feature_bbox = exactextract::geos_get_box(geos_context, geom.get());

                if (feature_bbox.intersects(values.grid().extent())) {
                    // Reduce value grid to portion overlapping feature
                    auto cropped_values_grid = values.grid().crop(feature_bbox);

                    if (use_weights) {
                        std::vector<RasterStats<double>> raster_stats;
                        raster_stats.reserve(weights.size());
                        for (int i = 0; i < weights.size(); i++) {
                            raster_stats.emplace_back(store_values);
                        }

                        // Crop weight grid to area where feature and weights are both defined
                        auto cropped_weights_grid = weights[0].grid().crop(feature_bbox);
                        auto cropped_common_grid = cropped_values_grid.overlapping_grid(cropped_weights_grid);

                        for (const auto &subgrid : subdivide(cropped_common_grid, max_cells_in_memory)) {
                            Raster<float> coverage = raster_cell_intersection(subgrid, geos_context, geom.get());

                            if (coverage.grid().empty()) {
                                continue;
                            }

                            Raster<double> values_cropped = values.read_box(subgrid.extent());
                            for (size_t i = 0; i < weights.size(); i++) {
                                Raster<double> weights_cropped = weights[i].read_box(subgrid.extent());
                                raster_stats[i].process(coverage, values_cropped, weights_cropped);

                                if (progress) std::cout << "." << std::flush;
                            }
                        }

                        write_stats_to_csv(name, raster_stats, stats, csvout);
                    } else {
                        RasterStats<double> raster_stats{store_values};

                        for (const auto &subgrid : subdivide(cropped_values_grid, max_cells_in_memory)) {
                            if (progress) std::cout << "." << std::flush;
                            Raster<float> coverage = raster_cell_intersection(subgrid, geos_context, geom.get());
                            Raster<double> values_cropped = values.read_box(subgrid.extent());

                            raster_stats.process(coverage, values_cropped);
                        }

                        write_stats_to_csv(name, raster_stats, stats, csvout);
                    }
                } else {
                    write_nas_to_csv(name, stats.size()*(std::max(1ul, weights.size())), csvout);
                }

            } catch (const std::exception & e) {
                std::cerr << e.what();
                if (progress) {
                    std::cout << "failed.";
                }
                failures.push_back(name);
            }
            if (progress) std::cout << std::endl;
        }
    }

    if (!failures.empty()) {
        std::cerr << "Failures:" << std::endl;
        for (const auto& name : failures) {
            std::cerr << name << std::endl;
        }
    }

    finishGEOS_r(geos_context);

    if (failures.empty()) {
        return 0;
    }

    return 1;
}
