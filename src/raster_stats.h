#ifndef RASTER_STATS_H
#define RASTER_STATS_H

#include "raster_cell_intersection.h"

float weighted_mean(const RasterCellIntersection & rci, const Matrix<float> & rast, float nodata) {
    double weights = 0;
    double weighted_vals = 0;

    size_t rows = rci.rows();
    size_t cols = rci.cols();

    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            float val = rast(i, j);
            float weight = rci.get_local(i, j);

            if (!std::isnan(val) && val != nodata) {
                weights += weight;
                weighted_vals += weight*val;
            }
        }
    }

    return (float) (weighted_vals / weights);
}

#endif