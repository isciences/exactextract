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

#include <geos_c.h>

#include "geos_utils.h"

#include "gdal.h"
#include "gdal_priv.h"
#include "cpl_conv.h"
#include "ogrsf_frmts.h"

#include "extent.h"
#include "raster_stats.h"
#include "raster_cell_intersection.h"

exactextract::Extent get_raster_extent(GDALDataset* rast) {
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
    const char* rast_filename = argc > 1 ? argv[1] : "/tmp/precip.tif";
    const char* poly_filename = argc > 2 ? argv[2] : "/tmp/ne_50m_admin_0_countries.shp";
    const char* field_name = argc > 3 ? argv[3] : "name";
    const char* output_filename = argc > 5 ? argv[4] : "output.csv";
    const char* filter = argc > 5 ? argv[5] : "";

    initGEOS(nullptr, nullptr);

    GDALAllRegister();
    GEOSContextHandle_t geos_context = OGRGeometry::createGEOSContext();

    GDALDataset* rast = (GDALDataset*) GDALOpen(rast_filename, GA_ReadOnly);
    GDALRasterBand* band = rast->GetRasterBand(1);
    GDALDataset* shp = (GDALDataset*) GDALOpenEx(poly_filename, GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
    OGRLayer* polys = shp->GetLayer(0);

    exactextract::Extent raster_extent = get_raster_extent(rast);
    exactextract::geom_ptr box = exactextract::geos_make_box_polygon(raster_extent.xmin, raster_extent.ymin, raster_extent.xmax, raster_extent.ymax);

    OGRFeature* feature;
    polys->ResetReading();

    std::ofstream csvout;
    csvout.open(output_filename);

    csvout << field_name << ",avg" << std::endl;;

    while ((feature = polys->GetNextFeature()) != nullptr) {
        std::string name{feature->GetFieldAsString(field_name)};

        if (strlen(filter) == 0 || name == filter) {
            std::cout << name << ": ";

            exactextract::geom_ptr geom { feature->GetGeometryRef()->exportToGEOS(geos_context), GEOSGeom_destroy };

            if (!GEOSContains(box.get(), geom.get())) {
                geom = { GEOSIntersection(box.get(), geom.get()), GEOSGeom_destroy };
            }

            try {
                exactextract::RasterCellIntersection rci(raster_extent, geom.get());

                exactextract::Matrix<float> m(rci.rows(), rci.cols());

                GDALRasterIO(band, GF_Read, rci.min_col(), rci.min_row(), rci.cols(), rci.rows(), m.data(), rci.cols(), rci.rows(), GDT_Float32, 0, 0);

                exactextract::RasterStats stats{rci, m};
                double weighted_mean = stats.mean();

                std::cout << weighted_mean;

                csvout << "\"" << name << "\"" << "," << weighted_mean << std::endl;
            } catch (...) {
                std::cout << "failed." << std::endl;
            }
        }
        OGRFeature::DestroyFeature(feature);
    }

    GDALClose(rast);
    GDALClose(shp);
}