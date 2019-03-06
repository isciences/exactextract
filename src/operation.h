// Copyright (c) 2019 ISciences, LLC.
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

#ifndef EXACTEXTRACT_OPERATION_H
#define EXACTEXTRACT_OPERATION_H

#include <string>

#include "grid.h"
#include "gdal_raster_wrapper.h"

namespace exactextract {

    class Operation {
    public:
        Operation(const std::string & p_stat, GDALRasterWrapper* p_values, GDALRasterWrapper* p_weights = nullptr) :
                stat{p_stat},
                values{p_values},
                weights{p_weights} {}

        bool weighted() const {
            return weights != nullptr;
        }

        Grid<bounded_extent> grid() const {
            if (weighted()) {
                return values->grid().common_grid(weights->grid());
            } else {
                return values->grid();
            }
        }

        std::string stat;
        GDALRasterWrapper* values;
        GDALRasterWrapper* weights;
    };

}

#endif //EXACTEXTRACT_OPERATION_H
