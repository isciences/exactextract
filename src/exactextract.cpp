// Copyright (c) 2018-2024 ISciences, LLC.
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
#include <stdexcept>
#include <string>
#include <vector>

#include "CLI11.hpp"

#include "deferred_gdal_writer.h"
#include "feature_sequential_processor.h"
#include "gdal_dataset_wrapper.h"
#include "gdal_raster_wrapper.h"
#include "gdal_writer.h"
#include "operation.h"
#include "processor.h"
#include "raster_sequential_processor.h"
#include "utils.h"
#include "utils_cli.h"
#include "version.h"
#include <ogr_srs_api.h>

#ifdef EE_PARALLEL
#include "raster_parallel_processor.h"
#endif

using exactextract::GDALDatasetWrapper;
using exactextract::GDALRasterWrapper;
using exactextract::Operation;

static GDALDatasetWrapper
load_dataset(const std::string& descriptor,
             const std::vector<std::string>& include_cols,
             const std::string& src_id_name,
             const std::string& dst_id_name,
             const std::string& dst_id_type);

static void
check_crs_consistent(const GDALDatasetWrapper& features, const std::vector<std::unique_ptr<exactextract::RasterSource>>& rasters, const std::vector<std::unique_ptr<exactextract::RasterSource>>& weights);

int
main(int argc, char** argv)
{
    CLI::App app{ "Zonal statistics using exactextract: version " + exactextract::version() };

    std::string poly_descriptor, src_id_name, output_filename, strategy, dst_id_type, dst_id_name;
    std::vector<std::string> stats;
    std::vector<std::string> raster_descriptors;
    std::vector<std::string> weight_descriptors;
    std::vector<std::string> include_cols;
    size_t max_cells_in_memory = 30;
    size_t threads = 4;

    bool progress = false;
    bool nested_output = false;
    bool include_geom = false;
    double grid_compat_tol = std::numeric_limits<double>::quiet_NaN();

    app.add_option("-p,--polygons", poly_descriptor, "polygon dataset")->required(true);
    app.add_option("-r,--raster", raster_descriptors, "raster dataset")->required(true);
    app.add_option("-w,--weights", weight_descriptors, "weighting dataset")->required(false);
    app.add_option("-f,--fid", src_id_name, "id from polygon dataset to retain in output")->required(false);
    app.add_option("-o,--output", output_filename, "output filename")->required(true);
    app.add_option("-s,--stat", stats, "statistics")->required(false)->expected(-1);
    app.add_option("--max-cells", max_cells_in_memory, "maximum number of raster cells to read in memory at once, in millions")->required(false)->default_val("30");
    app.add_option("--strategy", strategy, "processing strategy")->required(false)->default_val("feature-sequential");
    app.add_option("--threads", threads, "maximum number of parallel items that can processed at one time, default 4")->required(false)->default_val(4);
    app.add_option("--id-type", dst_id_type, "override type of id field in output")->required(false);
    app.add_option("--id-name", dst_id_name, "override name of id field in output")->required(false);
    app.add_flag("--nested-output", nested_output, "nested output");
    app.add_option("--include-col", include_cols, "columns from input to include in output");
    app.add_flag("--include-geom", include_geom, "include geometry in output");
    app.add_flag("--grid-compat-tol", grid_compat_tol, "grid compatibility tolerance");

    app.add_flag("--progress", progress);
    app.set_config("--config");

    if (argc == 1) {
        std::cout << app.help();
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "--version") {
        std::cout << "exactextract " << exactextract::version();
        std::cout << " (built against GDAL " << GDAL_VERSION_MAJOR << "." << GDAL_VERSION_MINOR << "." << GDAL_VERSION_REV << ")" << std::endl;
        return 0;
    }

    CLI11_PARSE(app, argc, argv)

    if (dst_id_name.empty() != dst_id_type.empty()) {
        std::cerr << "Must specify both --id_type and --id_name" << std::endl;
        return 1;
    }
    if (src_id_name.empty() && !dst_id_name.empty()) {
        src_id_name = dst_id_name;
    }

    max_cells_in_memory *= 1000000;

    std::unique_ptr<exactextract::Processor> proc;
    std::unique_ptr<exactextract::OutputWriter> writer;

    try {
        GDALAllRegister();
        OGRRegisterAll();

        auto rasters = exactextract::load_gdal_rasters(raster_descriptors);
        auto weights = exactextract::load_gdal_rasters(weight_descriptors);

        auto operations = prepare_operations(stats, rasters, weights);

        GDALDatasetWrapper shp = load_dataset(poly_descriptor, include_cols, src_id_name, dst_id_name, dst_id_type);

        check_crs_consistent(shp, rasters, weights);

        std::unique_ptr<exactextract::GDALWriter> gdal_writer = std::make_unique<exactextract::GDALWriter>(
          output_filename, !nested_output, shp.srs());

        if (!dst_id_name.empty()) {
            include_cols.insert(include_cols.begin(), dst_id_name);
        } else if (!src_id_name.empty()) {
            include_cols.insert(include_cols.begin(), src_id_name);
        }

        for (const auto& field : include_cols) {
            gdal_writer->copy_field(shp, field);
        }

        writer = std::move(gdal_writer);

        if (strategy == "feature-sequential") {
            proc = std::make_unique<exactextract::FeatureSequentialProcessor>(shp, *writer);
        } else if (strategy == "raster-sequential") {
            proc = std::make_unique<exactextract::RasterSequentialProcessor>(shp, *writer);
        } else if (strategy == "raster-parallel") {
#ifdef EE_PARALLEL
            proc = std::make_unique<exactextract::RasterParallelProcessor>(shp, *writer, threads);
#else
            throw std::runtime_error("Parallel processor not supported.");
#endif
        } else {
            throw std::runtime_error("Unknown processing strategy: " + strategy);
        }

        for (const auto& op : operations) {
            proc->add_operation(*op);
        }

        for (const auto& field : include_cols) {
            proc->include_col(field);
        }

        if (include_geom) {
            proc->include_geometry();
        }

        if (!std::isnan(grid_compat_tol)) {
            proc->set_grid_compat_tol(grid_compat_tol);
        }

        proc->set_max_cells_in_memory(max_cells_in_memory);
        proc->show_progress(progress);
        if (progress) {
            proc->set_progress_fn(exactextract::cli_progress);
        }

        proc->process();
        writer->finish();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;

        return 1;
    } catch (...) {
        std::cerr << "Unknown error." << std::endl;

        return 1;
    }
}

static GDALDatasetWrapper
load_dataset(const std::string& descriptor,
             const std::vector<std::string>& include_cols,
             const std::string& src_id_name,
             const std::string& dst_id_name,
             const std::string& dst_id_type)
{
    const auto parsed = exactextract::parse_dataset_descriptor(descriptor);

    std::vector<std::string> select;

    if (!src_id_name.empty()) {
        std::string id_select;

        if (!dst_id_type.empty()) {
            id_select += "CAST(";
        }
        id_select += src_id_name;

        if (!dst_id_type.empty()) {
            id_select += " AS " + dst_id_type + ")";
        }

        if (!dst_id_name.empty()) {
            id_select += " AS " + dst_id_name + "";
        }

        select.push_back(id_select);
    }

    for (const auto& col : include_cols) {
        select.push_back(col);
    }

    auto ds = GDALDatasetWrapper{ parsed.first, parsed.second };

    if (!select.empty()) {
        ds.set_select(select);
    }

    return ds;
}

static void
check_crs_consistent(const GDALDatasetWrapper& features, const std::vector<std::unique_ptr<exactextract::RasterSource>>& rasters, const std::vector<std::unique_ptr<exactextract::RasterSource>>& weights)
{
    OGRSpatialReferenceH expected = features.srs();
    for (const auto& raster : rasters) {
        OGRSpatialReferenceH srs = static_cast<const GDALRasterWrapper*>(raster.get())->srs();
        if (!expected) {
            expected = srs;
        }

        if (srs && expected && !OSRIsSame(srs, expected)) {
            std::cerr << "Input features/rasters do not have the same CRS" << std::endl;
        }
    }
    for (const auto& raster : weights) {
        OGRSpatialReferenceH srs = static_cast<const GDALRasterWrapper*>(raster.get())->srs();
        if (!expected) {
            expected = srs;
        }

        if (srs && expected && !OSRIsSame(srs, expected)) {
            std::cerr << "Input features/rasters do not have the same CRS" << std::endl;
        }
    }
}
