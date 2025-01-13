
// Copyright (c) 2024 ISciences, LLC.
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

#pragma once

#include "geos_utils.h"
#include "map_feature.h"
#include "processor.h"

namespace exactextract {

class RasterParallelProcessor : public Processor
{
  public:
    using Processor::Processor;

    void read_features();
    void populate_index();

    void process() override;

  private:
    std::vector<MapFeature> m_features;
    tree_ptr_r m_feature_tree{ geos_ptr(m_geos_context, GEOSSTRtree_create_r(m_geos_context, 10)) };
};

}
