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

#include "CLI11.hpp"

#include <geos_c.h>

#include "gdal.h"
#include "gdal_priv.h"
#include "cpl_conv.h"
#include "ogrsf_frmts.h"

#include "box.h"
#include "grid.h"
#include "geos_utils.h"
#include "raster.h"
#include "raster_stats.h"
#include "raster_cell_intersection.h"

using exactextract::Box;
using exactextract::Grid;
using exactextract::bounded_extent;
using exactextract::subdivide;
using exactextract::Raster;
using exactextract::RasterStats;
using exactextract::RasterView;

static Grid<bounded_extent> get_raster_grid(GDALDataset *rast) {
    double adfGeoTransform[6];
    if (rast->GetGeoTransform(adfGeoTransform) != CE_None) {
        throw std::runtime_error("Error reading transform");
    }

    double dx = std::abs(adfGeoTransform[1]);
    double dy = std::abs(adfGeoTransform[5]);
    double ulx = adfGeoTransform[0];
    double uly = adfGeoTransform[3];

    int nx = rast->GetRasterXSize();
    int ny = rast->GetRasterYSize();

    return {{
        ulx,
        uly - ny*dy,
        ulx + nx*dx,
        uly},
        dx, 
        dy
    };
}

static Raster<double> read_box(GDALRasterBand* band, const Grid<bounded_extent> & grid, const Box & box, double* nodata) {
    auto cropped_grid = grid.shrink_to_fit(box);
    Raster<double> vals(cropped_grid);
    if (nodata != nullptr) {
        vals.set_nodata(*nodata);
    }

    auto error = GDALRasterIO(band,
                              GF_Read,
                              (int) cropped_grid.col_offset(grid),
                              (int) cropped_grid.row_offset(grid),
                              (int) cropped_grid.cols(),
                              (int) cropped_grid.rows(),
                              vals.data().data(),
                              (int) cropped_grid.cols(),
                              (int) cropped_grid.rows(),
                              GDT_Float64,
                              0,
                              0);

    if (error) {
        throw std::runtime_error("Error reading from raster.");
    }

    return vals;
}

static bool stored_values_needed(const std::vector<std::string> & stats) {
    for (const auto& stat : stats) {
        if (stat == "mode" || stat == "majority" || stat == "minority" || stat == "variety")
            return true;
    }
    return false;
}

static void write_stats_to_csv(const std::string & name, const RasterStats<double> & raster_stats, const std::vector<std::string> & stats, std::ostream & csvout) {
    csvout << name;
    for (const auto& stat : stats) {
        csvout << ",";
        if (stat == "mean") {
            csvout << raster_stats.mean();
        } else if (stat == "count") {
            csvout << raster_stats.count();
        } else if (stat == "sum") {
            csvout << raster_stats.sum();
        } else if (stat == "variety") {
            csvout << raster_stats.variety();
        } else if (stat == "weighted mean") {
            csvout << raster_stats.weighted_mean();
        } else if (stat == "weighted count") {
            csvout << raster_stats.weighted_count();
        } else if (stat == "weighted sum") {
            csvout << raster_stats.weighted_sum();
        } else if (stat == "weighted fraction") {
            csvout << raster_stats.weighted_fraction();
        } else if (stat == "min") {
            if (raster_stats.count() > 0) {
                csvout << raster_stats.min();
            } else {
                csvout << "NA";
            }
        } else if (stat == "max") {
            if (raster_stats.count() > 0) {
                csvout << raster_stats.max();
            } else {
                csvout << "NA";
            }
        } else if (stat == "mode") {
            if (raster_stats.count() > 0) {
                csvout << raster_stats.mode();
            } else {
                csvout << "NA";
            }
        } else if (stat == "minority") {
            if (raster_stats.count() > 0) {
                csvout << raster_stats.minority();
            } else {
                csvout << "NA";
            }
        } else {
            throw std::runtime_error("Unknown stat: " + stat);
        }
    }
    csvout << std::endl;

}

static void write_csv_header(const std::string & field_name, const std::vector<std::string> & stats, std::ostream & csvout) {
    csvout << field_name;
    for (const auto& stat : stats) {
        csvout << "," << stat;
    }
    csvout << std::endl;
}

int main(int argc, char** argv) {
    CLI::App app{"Zonal statistics using exactextract"};

    std::string poly_filename, rast_filename, weights_filename, field_name, output_filename, filter;
    std::vector<std::string> stats;
    size_t max_cells_in_memory = 30;
    bool progress;
    app.add_option("-p", poly_filename, "polygon dataset")->required(true);
    app.add_option("-r", rast_filename, "raster values dataset")->required(true);
    app.add_option("-w", weights_filename, "optional raster weights dataset")->required(false);
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
    bool use_weights = !weights_filename.empty();
    max_cells_in_memory *= 1000000;

    initGEOS(nullptr, nullptr);

    GDALAllRegister();
    GEOSContextHandle_t geos_context = OGRGeometry::createGEOSContext();

    // Open GDAL datasets for out inputs
    int values_nodata;
    GDALDataset* rast = (GDALDataset*) GDALOpen(rast_filename.c_str(), GA_ReadOnly);
    if (!rast) {
        std::cerr << "Failed to open " << rast_filename << std::endl;
        return 1;
    }
    GDALRasterBand* band = rast->GetRasterBand(1);
    double values_nodata_value = GDALGetRasterNoDataValue(band, &values_nodata);

    GDALDataset* weights_rast = nullptr;
    GDALRasterBand* weights_band = nullptr;
    int weights_nodata = false;
    double weights_nodata_value;
    if (use_weights) {
        weights_rast = (GDALDataset*) GDALOpen(weights_filename.c_str(), GA_ReadOnly);
        if (!weights_rast) {
            std::cerr << "Failed to open " << rast_filename << std::endl;
            return 1;
        }
        weights_band = weights_rast->GetRasterBand(1);
        weights_nodata_value = GDALGetRasterNoDataValue(weights_band, &weights_nodata);
    }

    GDALDataset* shp = (GDALDataset*) GDALOpenEx(poly_filename.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);

    if (!shp) {
        std::cerr << "Failed to open " << poly_filename << std::endl;
        return 1;
    }

    OGRLayer* polys = shp->GetLayer(0);
    polys->ResetReading();

    auto values_grid = get_raster_grid(rast);
    auto weights_grid = use_weights ? get_raster_grid(weights_rast) : values_grid;

    if (!values_grid.compatible_with(weights_grid)) {
        std::cerr << "Value and weighting rasters do not have compatible grids." << std::endl;
        std::cerr << "Value grid origin: (" << values_grid.xmin() << "," << values_grid.ymin() << ") resolution: (";
        std::cerr << values_grid.dx() << "," << values_grid.dy() << ")" << std::endl;
        std::cerr << "Weighting grid origin: (" << weights_grid.xmin() << "," << weights_grid.ymin() << ") resolution: (";
        std::cerr << weights_grid.dx() << "," << weights_grid.dy() << ")" << std::endl;
        return 1;
    }

    OGRFeature* feature;

    std::ofstream csvout;
    csvout.open(output_filename);
    write_csv_header(field_name, stats, csvout);

    bool store_values = stored_values_needed(stats);
    std::vector<std::string> failures;
    while ((feature = polys->GetNextFeature()) != nullptr) {
        std::string name{feature->GetFieldAsString(field_name.c_str())};

        if (filter.length() == 0 || name == filter) {
            exactextract::geom_ptr geom { feature->GetGeometryRef()->exportToGEOS(geos_context), GEOSGeom_destroy };
            if (progress) std::cout << "Processing " << name << std::flush;

            try {
                Box bbox = exactextract::geos_get_box(geom.get());
                RasterStats<double> raster_stats{store_values};

                if (bbox.intersects(values_grid.extent())) {
                    auto cropped_values_grid = values_grid.shrink_to_fit(bbox.intersection(values_grid.extent()));

                    if (use_weights) {
                        auto cropped_weights_grid = values_grid.shrink_to_fit(bbox.intersection(values_grid.extent()));
                        auto cropped_common_grid = cropped_values_grid.common_grid(cropped_weights_grid);

                        for (const auto& subgrid : subdivide(cropped_common_grid, max_cells_in_memory)) {
                            if (progress) std::cout << "." << std::flush;
                            Raster<float> coverage = raster_cell_intersection(subgrid, geom.get());

                            Raster<double> values = read_box(band, values_grid, subgrid.extent(), values_nodata ? &values_nodata_value : nullptr);
                            Raster<double> weights = read_box(weights_band, weights_grid, subgrid.extent(), weights_nodata ? &weights_nodata_value :nullptr);

                            raster_stats.process(coverage, values, weights);
                        }
                    } else {
                        for (const auto &subgrid : subdivide(cropped_values_grid, max_cells_in_memory)) {
                            if (progress) std::cout << "." << std::flush;
                            Raster<float> coverage = raster_cell_intersection(subgrid, geom.get());
                            Raster<double> values = read_box(band, values_grid, subgrid.extent(), values_nodata ? &values_nodata_value : nullptr);

                            raster_stats.process(coverage, values);
                        }
                    }

                    write_stats_to_csv(name, raster_stats, stats, csvout);
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
        OGRFeature::DestroyFeature(feature);
    }

    if (!failures.empty()) {
        std::cerr << "Failures:" << std::endl;
        for (const auto& name : failures) {
            std::cerr << name << std::endl;
        }
    }

    GDALClose(rast);
    if (use_weights) {
        GDALClose(weights_rast);
    }
    GDALClose(shp);
    finishGEOS();

    if (failures.empty()) {
        return 0;
    }

    return 1;
}
