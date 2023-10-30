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

#include <memory>

#ifndef EXACTEXTRACT_RASTER_SEQUENTIAL_PROCESSOR_H
#define EXACTEXTRACT_RASTER_SEQUENTIAL_PROCESSOR_H


#include "map_feature.h"
#include "geos_utils.h"
#include "processor.h"

namespace exactextract {

    /**
     * @brief The RasterSequentialProcessor class iterates over chunks of the raster, fetching features that intersect each
     * chunk and incrementally updating their statistics. It requires that all features and their associated statistics
     * remain in memory at all times. It may be efficient for rasters that from which random rectangles cannot be
     * efficiently read.
     */
    class RasterSequentialProcessor : public Processor {
    public:
        using Processor::Processor;

        void read_features();
        void populate_index();

        void process() override;

    private:
        std::vector<MapFeature> m_features;
        tree_ptr_r m_feature_tree{geos_ptr(m_geos_context, GEOSSTRtree_create_r(m_geos_context, 10))};
    };

}

#endif //EXACTEXTRACT_RASTER_SEQUENTIAL_PROCESSOR_H
