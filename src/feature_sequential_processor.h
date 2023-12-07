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

#ifndef EXACTEXTRACT_FEATURE_SEQUENTIAL_PROCESSOR_H
#define EXACTEXTRACT_FEATURE_SEQUENTIAL_PROCESSOR_H

#include "processor.h"

namespace exactextract {

/**
 * @brief The FeatureSequentialProcessor class iterates over the features in the input dataset, reading the necessary
 * raster cells to complete the specified Operation on the feature, then writing those results before continuing onto
 * the next feature. It is best-suited to raster datasets from which arbitrary rectangles can be read efficiently.
 */
class FeatureSequentialProcessor : public Processor
{
  public:
    using Processor::Processor;

    void process() override;
};
}

#endif
