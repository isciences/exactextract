#include <pybind11/pybind11.h>

#include "box_bindings.h"
#include "cell_bindings.h"
#include "coordinate_bindings.h"
#include "gdal_dataset_wrapper_bindings.h"
#include "gdal_raster_wrapper_bindings.h"
#include "grid_bindings.h"
#include "operation_bindings.h"
#include "processor_bindings.h"
#include "side_bindings.h"
#include "utils_bindings.h"
#include "writer_bindings.h"

namespace py = pybind11;

namespace exactextract
{
    PYBIND11_MODULE(_exactextract, m)
    {
        bind_box(m);
        bind_cell(m);
        bind_coordinate(m);
        bind_gdal_dataset_wrapper(m);
        bind_gdal_raster_wrapper(m);
        bind_grid(m);
        bind_operation(m);
        bind_processor(m);
        bind_side(m);
        bind_utils(m);
        bind_writer(m);
    }
}