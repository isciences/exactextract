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
#include "version.h"

#include "raster_sequential_processor.h"
#include "csv_writer.h"

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
    CLI::App app{"Zonal statistics using exactextract: build " + exactextract::version()};

    std::string poly_filename, field_name, output_filename, filter, strategy;
    std::vector<std::string> stats;
    std::vector<std::string> raster_filenames;
    size_t max_cells_in_memory = 30;
    bool progress;
    app.add_option("-p", poly_filename, "polygon dataset")->required(true);
    app.add_option("-r", raster_filenames, "raster dataset")->required(true);
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
    CLI11_PARSE(app, argc, argv);
    max_cells_in_memory *= 1000000;

    GDALAllRegister();

    std::unordered_map<std::string, GDALRasterWrapper> rasters;

    for (const auto& filename : raster_filenames) {
        // TODO take dataset name somehow.
        // TODO take band number somehow
        rasters.emplace(filename, GDALRasterWrapper{filename, 1});
    }

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

    bool store_values = stored_values_needed(stats);
    std::vector<std::string> failures;

    exactextract::CSVWriter writer(output_filename, field_name);
    //writer.add_var("_mean");

    strategy = "raster-sequential";

    if (strategy == "feature-sequential") {
        auto proc = exactextract::FeatureSequentialProcessor(shp, writer, field_name);
        proc.add_operation({"mean", &rasters.at(raster_filenames[0])});
        proc.add_operation({"sum", &rasters.at(raster_filenames[0])});
        proc.add_operation({"sum", &rasters.at(raster_filenames[1])});
        proc.process();
    }
    if (strategy == "raster-sequential") {
        auto proc = exactextract::RasterSequentialProcessor(shp, writer, field_name);
        proc.add_operation({"mean", &rasters.at(raster_filenames[0])});
        proc.add_operation({"sum", &rasters.at(raster_filenames[0])});
        proc.add_operation({"sum", &rasters.at(raster_filenames[1])});
        proc.process();
    }


}
