// Copyright (c) 2023 ISciences, LLC.
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

#include <chrono>
#include <gdal.h>
#include <geos_c.h>

#include "CLI11.hpp"
#include "grid.h"
#include "raster_cell_intersection.h"
#include "version.h"

void print_message(const char* message, void*) {
    std::cerr << message << std::endl;
}

int main(int argc, char** argv) {
    std::array<double, 4> te;
    std::array<double, 2> tr;
    std::string src;
    std::string dst;
    std::string output_driver;

    CLI::App app{"Polygon subdivision using exactextract: version " + exactextract::version()};

    app.add_option("--te", te, "grid extent")->required(true)->delimiter(' ');
    app.add_option("--tr", tr, "grid resolution")->required(true)->delimiter(' ');
    app.add_option("-f", output_driver, "output driver")->required(false)->default_val("ESRI Shapefile");
    app.add_option("src", src, "src")->required(true);
    app.add_option("dst", dst, "dst")->required(true);

    if (argc == 1) {
        std::cout << app.help();
        return 0;
    }
    CLI11_PARSE(app, argc, argv)

    GDALAllRegister();

    GDALDatasetH ds = GDALOpenEx(src.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
    OGRLayerH lyr = GDALDatasetGetLayer(ds, 0);
    OGRSpatialReferenceH srs = OGR_L_GetSpatialRef(lyr);

    auto dstDriver = GDALGetDriverByName(output_driver.c_str());
    if (dstDriver == nullptr) {
        std::cerr << "Unknown format: \"" << output_driver << "\"" << std::endl;
        return 1;
    }

    OGRDataSourceH ds_out = GDALCreate(dstDriver, dst.c_str(), 0, 0, 0, GDT_Unknown, nullptr);

    OGRLayerH lyr_out = GDALDatasetCreateLayer(ds_out, "out", NULL, wkbMultiPolygon, nullptr);

    GEOSContextHandle_t geos_context = initGEOS_r(nullptr, nullptr);
    GEOSContext_setErrorMessageHandler_r(geos_context, print_message, nullptr);
    GEOSContext_setNoticeMessageHandler_r(geos_context, print_message, nullptr);

    OGRFeatureH feature = OGR_L_GetNextFeature(lyr);

    std::chrono::milliseconds runtime(0);
    exactextract::Box box (te[0], te[1], te[2], te[3]);
    exactextract::Grid<exactextract::bounded_extent> grid(box, tr[0], tr[1]);

    std::size_t features = 0;
    while (feature != nullptr) {
        OGRGeometryH geom = OGR_F_GetGeometryRef(feature);

        auto sz = static_cast<size_t>(OGR_G_WkbSize(geom));
        auto buff = std::make_unique<unsigned char[]>(sz);
        OGR_G_ExportToWkb(geom, wkbXDR, buff.get());

        GEOSGeometry* g = GEOSGeomFromWKB_buf_r(geos_context, buff.get(), sz);

        OGREnvelope e;
        OGR_G_GetEnvelope(geom, &e);

        auto start = std::chrono::high_resolution_clock::now();
        auto result = exactextract::RasterCellIntersection::subdivide_polygon(grid, geos_context, g);
        auto stop = std::chrono::high_resolution_clock::now();

        runtime += std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

        auto ngeoms = GEOSGetNumGeometries_r(geos_context, result.get());
        for (int i = 0; i < ngeoms; i++) {

            std::size_t wkbSize;
            unsigned char* wkb = GEOSGeomToWKB_buf_r(geos_context, GEOSGetGeometryN_r(geos_context, result.get(), i), &wkbSize);
            OGRGeometryH ogr_geom;
            OGR_G_CreateFromWkb(wkb, srs, &ogr_geom, -1);

            OGRFeatureH feature = OGR_F_Create(OGR_L_GetLayerDefn(lyr_out));
            OGR_F_SetGeometryDirectly(feature, ogr_geom);

            auto err = OGR_L_CreateFeature(lyr_out, feature);

            OGR_F_Destroy(feature);
        }

        feature = OGR_L_GetNextFeature(lyr);
        features++;
    }

    GDALClose(ds_out);

    std::cerr << "Subdivided " << features << " features with " << grid.size() << " cells in " << runtime.count() << " ms" << std::endl;

    return 0;
}
