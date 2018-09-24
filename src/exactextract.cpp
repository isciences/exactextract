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

static Raster<double> read_box(GDALRasterBand* band, const Grid<bounded_extent> & grid, const Box & box) {
    auto cropped_grid = grid.shrink_to_fit(box);
    Raster<double> vals(cropped_grid);

    // TODO check return value
    GDALRasterIO(band,
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

    return vals;
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
                csvout << raster_stats.max();
            } else {
                csvout << "NA";
            }
        } else if (stat == "minority") {
            if (raster_stats.count() > 0) {
                csvout << raster_stats.max();
            } else {
                csvout << "NA";
            }
        } else {
            throw std::runtime_error("Unknown stat: " + stat);
        }
    }
    csvout << std::endl;

}

int main(int argc, char** argv) {
    CLI::App app{"Zonal statistics using exactextract"};

    std::string poly_filename, rast_filename, weights_filename, field_name, output_filename, filter;
    std::vector<std::string> stats;
    size_t max_cells_in_memory = 30;
    app.add_option("-p", poly_filename, "polygon dataset")->required(true);
    app.add_option("-r", rast_filename, "raster values dataset")->required(true);
    app.add_option("-w", weights_filename, "optional raster weights dataset")->required(false);
    app.add_option("-f", field_name, "id from polygon dataset to retain in output")->required(true);
    app.add_option("-o", output_filename, "output filename")->required(true);
    app.add_option("-s", stats, "statistics")->required(true)->expected(-1);
    app.add_option("--filter", filter, "only process specified value of id")->required(false);
    app.add_option("--max-cells", max_cells_in_memory, "maximum number of raster cells to read in memory at once, in millions")->required(false)->default_val("30");

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

    GDALDataset* rast = (GDALDataset*) GDALOpen(rast_filename.c_str(), GA_ReadOnly);
    if (!rast) {
        std::cerr << "Failed to open " << rast_filename << std::endl;
        return 1;
    }
    GDALRasterBand* band = rast->GetRasterBand(1);

    GDALDataset* weights_rast = nullptr;
    GDALRasterBand* weights_band = nullptr;
    if (use_weights) {
        weights_rast = (GDALDataset*) GDALOpen(weights_filename.c_str(), GA_ReadOnly);
        if (!weights_rast) {
            std::cerr << "Failed to open " << rast_filename << std::endl;
            return 1;
        }
        weights_band = weights_rast->GetRasterBand(1);
    }

    GDALDataset* shp = (GDALDataset*) GDALOpenEx(poly_filename.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);

    if (!shp) {
        std::cerr << "Failed to open " << poly_filename << std::endl;
        return 1;
    }

    OGRLayer* polys = shp->GetLayer(0);

    // TODO handle nodata for weights
    int has_nodata;
    double nodata = GDALGetRasterNoDataValue(band, &has_nodata);

    std::cout << "Using NODATA value of " << nodata << std::endl;

    // TODO figure out how to handle differing extents for weights and values (when it matters)
    auto values_grid = get_raster_grid(rast);
    auto weights_grid = use_weights ? get_raster_grid(weights_rast) : values_grid;

    exactextract::geom_ptr box = exactextract::geos_make_box_polygon(values_grid.xmin(), values_grid.ymin(), values_grid.xmax(), values_grid.ymax());

    OGRFeature* feature;
    polys->ResetReading();

    std::ofstream csvout;
    csvout.open(output_filename);

    csvout << field_name;
    for (const auto& stat : stats) {
        csvout << "," << stat;
    }
    csvout << std::endl;

    std::vector<std::string> failures;

    while ((feature = polys->GetNextFeature()) != nullptr) {
        std::string name{feature->GetFieldAsString(field_name.c_str())};

        if (filter.length() == 0 || name == filter) {
            exactextract::geom_ptr geom { feature->GetGeometryRef()->exportToGEOS(geos_context), GEOSGeom_destroy };
            std::cout << "Processing " << name << std::endl;

            try {
                Box bbox = exactextract::geos_get_box(geom.get());
                RasterStats<double> raster_stats;

                if (bbox.intersects(values_grid.extent())) {
                    auto cropped_values_grid = values_grid.shrink_to_fit(bbox.intersection(values_grid.extent()));

                    if (use_weights) {
                        auto cropped_weights_grid = values_grid.shrink_to_fit(bbox.intersection(values_grid.extent()));

                        auto cropped_common_grid = cropped_values_grid.common_grid(cropped_weights_grid);

                        for (const auto& subgrid : subdivide(cropped_common_grid, max_cells_in_memory)) {
                            Raster<double> values = read_box(band, values_grid, subgrid.extent());
                            Raster<double> weights = read_box(weights_band, weights_grid, subgrid.extent());

                            Raster<float> coverage = raster_cell_intersection(subgrid, geom.get());

                            raster_stats.process(coverage, values, weights, has_nodata ? &nodata : nullptr);
                        }
                    } else {
                        for (const auto &subgrid : subdivide(cropped_values_grid, max_cells_in_memory)) {
                            Raster<float> coverage = raster_cell_intersection(subgrid, geom.get());
                            Raster<double> vals_subgrid = read_box(band, values_grid, subgrid.extent());

                            raster_stats.process(coverage, vals_subgrid, has_nodata ? &nodata : nullptr);
                        }
                    }

                    write_stats_to_csv(name, raster_stats, stats, csvout);
                }

            } catch (...) {
                std::cout << "failed";
                failures.push_back(name);
            }
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
    GDALClose(shp);
    finishGEOS();

    if (failures.empty()) {
        return 0;
    }

    return 1;
}
