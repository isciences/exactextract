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

#include "extent.h"
#include "geos_utils.h"
#include "raster.h"
#include "raster_stats.h"
#include "raster_cell_intersection.h"

static exactextract::Extent get_raster_extent(GDALDataset* rast) {
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

    return {
        ulx,
        uly - ny*dy,
        ulx + nx*dx,
        uly,
        dx, 
        dy
    };
}

int main(int argc, char** argv) {
    CLI::App app{"Zonal statistics using exactextract"};

    std::string poly_filename, rast_filename, field_name, output_filename, filter;
    std::vector<std::string> stats;
    app.add_option("-p", poly_filename, "polygon dataset")->required(true);
    app.add_option("-r", rast_filename, "raster dataset")->required(true);
    app.add_option("-f", field_name, "id from polygon dataset to retain in output")->required(true);
    app.add_option("-o", output_filename, "output filename")->required(true);
    app.add_option("-s", stats, "statistics")->required(true)->expected(-1);
    app.add_option("--filter", filter, "only process specified value of id")->required(false);

    if (argc == 1) {
        std::cout << app.help();
        return 0;
    }
    CLI11_PARSE(app, argc, argv);

    initGEOS(nullptr, nullptr);

    GDALAllRegister();
    GEOSContextHandle_t geos_context = OGRGeometry::createGEOSContext();

    GDALDataset* rast = (GDALDataset*) GDALOpen(rast_filename.c_str(), GA_ReadOnly);

    if (!rast) {
        std::cerr << "Failed to open " << rast_filename << std::endl;
        return 1;
    }

    GDALDataset* shp = (GDALDataset*) GDALOpenEx(poly_filename.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);

    if (!shp) {
        std::cerr << "Failed to open " << poly_filename << std::endl;
        return 1;
    }

    GDALRasterBand* band = rast->GetRasterBand(1);
    OGRLayer* polys = shp->GetLayer(0);

    int has_nodata;
    double nodata = GDALGetRasterNoDataValue(band, &has_nodata);

    std::cout << "Using NODATA value of " << nodata << std::endl;

    exactextract::Extent raster_extent = get_raster_extent(rast);
    exactextract::geom_ptr box = exactextract::geos_make_box_polygon(raster_extent.xmin, raster_extent.ymin, raster_extent.xmax, raster_extent.ymax);

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

            if (!GEOSContains(box.get(), geom.get())) {
                geom = { GEOSIntersection(box.get(), geom.get()), GEOSGeom_destroy };
            }

            try {
                exactextract::Raster<float> coverage = raster_cell_intersection(raster_extent, geom.get());

                exactextract::Raster<double> vals(coverage.extent());
                GDALRasterIO(band,
                        GF_Read,
                        (int) coverage.extent().col_offset(raster_extent),
                        (int) coverage.extent().row_offset(raster_extent),
                        (int) vals.extent().cols(),
                        (int) vals.extent().rows(),
                        vals.data().data(),
                        (int) vals.extent().cols(),
                        (int) vals.extent().rows(),
                        GDT_Float64,
                        0,
                        0);

                exactextract::RasterStats<double> raster_stats{coverage, vals, has_nodata ? &nodata : nullptr};


                //exactextract::RasterCellIntersection rci(raster_extent, geom.get());

                //exactextract::Matrix<double> m(rci.rows(), rci.cols());

                //GDALRasterIO(band, GF_Read, rci.min_col(), rci.min_row(), rci.cols(), rci.rows(), m.data(), rci.cols(), rci.rows(), GDT_Float64, 0, 0);

                //exactextract::RasterStats<decltype(m)> raster_stats{coverage, m, true, has_nodata ? &nodata : nullptr};

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
            } catch (...) {
                failures.push_back(name);
                std::cout << "failed." << std::endl;
            }
        }
        OGRFeature::DestroyFeature(feature);

        if (!failures.empty()) {
            std::cerr << "Failures:" << std::endl;
            for (const auto& name : failures) {
                std::cerr << name << std::endl;
            }
        }
    }

    GDALClose(rast);
    GDALClose(shp);

    if (failures.empty()) {
        return 0;
    }

    return 1;
}