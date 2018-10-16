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

static void write_stats_to_csv(const std::string & name, const std::vector<RasterStats<double>> & raster_stats, const std::vector<std::string> & stats, std::ostream & csvout) {
    write_stats_to_csv(name, raster_stats[0], stats, csvout);
    for (size_t i = 1; i < raster_stats.size(); i++) {
        write_stats_to_csv("", raster_stats[i], stats, csvout);
    }
}

static void write_csv_header(const std::string & field_name, const std::vector<std::string> & stats, std::ostream & csvout) {
    csvout << field_name;
    for (const auto& stat : stats) {
        csvout << "," << stat;
    }
    csvout << std::endl;
}

class GDALRasterInfo {

public:
    GDALRasterInfo(const std::string & filename, int bandnum) : m_grid{Grid<bounded_extent>::make_empty()}{
        auto rast = GDALOpen(filename.c_str(), GA_ReadOnly);
        if (!rast) {
            throw std::runtime_error("Failed to open " + filename);
        }

        int has_nodata;
        auto band = GDALGetRasterBand(rast, bandnum);
        double nodata_value = GDALGetRasterNoDataValue(band, &has_nodata);

        m_rast = rast;
        m_band = band;
        m_nodata_value = nodata_value;
        m_has_nodata = static_cast<bool>(has_nodata);
        compute_raster_grid();
    }

    const Grid<bounded_extent>& grid() const {
        return m_grid;
    }

    Raster<double> read_box(const Box & box) {
        auto cropped_grid = m_grid.shrink_to_fit(box);
        Raster<double> vals(cropped_grid);
        if (m_has_nodata) {
            vals.set_nodata(m_nodata_value);
        }

        auto error = GDALRasterIO(m_band,
                                  GF_Read,
                                  (int) cropped_grid.col_offset(m_grid),
                                  (int) cropped_grid.row_offset(m_grid),
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

    ~GDALRasterInfo() {
        GDALClose(m_rast);
    }

    GDALRasterInfo(const GDALRasterInfo&) = delete;
    GDALRasterInfo(GDALRasterInfo&&) = default;


private:
    GDALDatasetH m_rast;
    GDALRasterBandH m_band;
    double m_nodata_value;
    bool m_has_nodata;
    Grid<bounded_extent> m_grid;

    Grid<bounded_extent> compute_raster_grid() {
        double adfGeoTransform[6];
        if (GDALGetGeoTransform(m_rast, adfGeoTransform) != CE_None) {
            throw std::runtime_error("Error reading transform");
        }

        double dx = std::abs(adfGeoTransform[1]);
        double dy = std::abs(adfGeoTransform[5]);
        double ulx = adfGeoTransform[0];
        double uly = adfGeoTransform[3];

        int nx = GDALGetRasterXSize(m_rast);
        int ny = GDALGetRasterYSize(m_rast);

        Box box{ulx,
                uly - ny*dy,
                ulx + nx*dx,
                uly};

        m_grid = {box, dx, dy};
    }

};

class GDALDatasetInfo {
public:
    GDALDatasetInfo(const std::string & filename, int layer) {
        m_dataset = GDALOpenEx(filename.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
        if (m_dataset == nullptr) {
            throw std::runtime_error("Failed to open " + filename);
        }

        m_layer = GDALDatasetGetLayer(m_dataset, layer);
        OGR_L_ResetReading(m_layer);
        m_feature = NULL;
    }

    bool next() {
        if (m_feature != NULL) {
            OGR_F_Destroy(m_feature);
        }
        m_feature = OGR_L_GetNextFeature(m_layer);
        return m_feature != NULL;
    }

    GEOSGeometry* feature_geometry(const GEOSContextHandle_t & geos_context) {
        OGRGeometryH geom = OGR_F_GetGeometryRef(m_feature);

        int sz = OGR_G_WkbSize(geom);
        auto buff = std::make_unique<unsigned char[]>(sz);
        OGR_G_ExportToWkb(geom, wkbXDR, buff.get());

        return GEOSGeomFromWKB_buf(buff.get(), sz);
    }

    std::string feature_field(const std::string & field_name) {
        int index = OGR_F_GetFieldIndex(m_feature, field_name.c_str());
        // TODO check handling of invalid field name
        return OGR_F_GetFieldAsString(m_feature, index);
    }

    ~GDALDatasetInfo() {
        GDALClose(m_dataset);

        if (m_feature != NULL) {
            OGR_F_Destroy(m_feature);
        }
    }

private:
    GDALDatasetH m_dataset;
    OGRFeatureH m_feature;
    OGRLayerH m_layer;
};

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

    initGEOS(nullptr, nullptr);

    GDALAllRegister();
    GEOSContextHandle_t geos_context = initGEOS_r(nullptr, nullptr);

    // Open GDAL datasets for out inputs
    GDALRasterInfo values{rast_filename, 1};

    std::vector<GDALRasterInfo> weights;

    for (const auto& filename : weights_filenames) {
        weights.emplace_back(filename, 1);
    }

    GDALDatasetInfo shp{poly_filename, 0};

    // Check grid compatibility
    for (const auto& w : weights) {
        if (!values.grid().compatible_with(w.grid())) {
            std::cerr << "Value and weighting rasters do not have compatible grids." << std::endl;
            std::cerr << "Value grid origin: (" << values.grid().xmin() << "," << values.grid().ymin() << ") resolution: (";
            std::cerr << values.grid().dx() << "," << values.grid().dy() << ")" << std::endl;
            std::cerr << "Weighting grid origin: (" << w.grid().xmin() << "," << w.grid().ymin() << ") resolution: (";
            std::cerr << w.grid().dx() << "," << w.grid().dy() << ")" << std::endl;
            return 1;
        }
    }

    std::ofstream csvout;
    csvout.open(output_filename);
    write_csv_header(field_name, stats, csvout);

    bool store_values = stored_values_needed(stats);
    std::vector<std::string> failures;
    while (shp.next()) {
        std::string name{shp.feature_field(field_name)};

        if (filter.length() == 0 || name == filter) {
            auto deleter = [&geos_context](GEOSGeometry* g) { GEOSGeom_destroy_r(geos_context, g); };
            std::unique_ptr<GEOSGeometry, decltype(deleter)> geom{shp.feature_geometry(geos_context), deleter};

            if (progress) std::cout << "Processing " << name << std::flush;

            try {
                Box bbox = exactextract::geos_get_box(geom.get());

                if (bbox.intersects(values.grid().extent())) {
                    auto cropped_values_grid = values.grid().shrink_to_fit(bbox.intersection(values.grid().extent()));

                    if (use_weights) {
                        std::vector<RasterStats<double>> raster_stats;
                        raster_stats.reserve(weights.size());
                        for (int i = 0; i < weights.size(); i++) {
                            raster_stats.emplace_back(store_values);
                        }

                        for (size_t i = 0; i < weights.size(); i++) {
                            auto& w = weights[i];
                            auto cropped_weights_grid = values.grid().shrink_to_fit(bbox.intersection(values.grid().extent()));
                            auto cropped_common_grid = cropped_values_grid.common_grid(cropped_weights_grid);

                            for (const auto &subgrid : subdivide(cropped_common_grid, max_cells_in_memory)) {
                                if (progress) std::cout << "." << std::flush;
                                Raster<float> coverage = raster_cell_intersection(subgrid, geom.get());

                                Raster<double> values_cropped = values.read_box(subgrid.extent());
                                Raster<double> weights_cropped = w.read_box(subgrid.extent());

                                raster_stats[i].process(coverage, values_cropped, weights_cropped);
                            }

                            write_stats_to_csv(name, raster_stats, stats, csvout);
                        }
                    } else {
                        RasterStats<double> raster_stats{store_values};

                        for (const auto &subgrid : subdivide(cropped_values_grid, max_cells_in_memory)) {
                            if (progress) std::cout << "." << std::flush;
                            Raster<float> coverage = raster_cell_intersection(subgrid, geom.get());
                            Raster<double> values_cropped = values.read_box(subgrid.extent());

                            raster_stats.process(coverage, values_cropped);
                        }

                        write_stats_to_csv(name, raster_stats, stats, csvout);
                    }
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

    finishGEOS();
    finishGEOS_r(geos_context);

    if (failures.empty()) {
        return 0;
    }

    return 1;
}
