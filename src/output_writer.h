// Copyright (c) 2019-2023 ISciences, LLC.
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

#ifndef EXACTEXTRACT_OUTPUT_WRITER_H
#define EXACTEXTRACT_OUTPUT_WRITER_H

#include "feature.h"

#include <memory>
#include <string>

namespace exactextract {

class Operation;

class OutputWriter
{
  public:
    /// Create a new feature to which attributes can be assigned
    /// An implementation may override this to that the provided
    /// feature is of a class that can be efficiently consumed
    /// by the `write` method.
    virtual std::unique_ptr<Feature> create_feature();

    /// Write the provided feature
    virtual void write(const Feature&) = 0;

    /// Method to be called for each `Operation` whose results will
    /// be written. May be used to create necessary fields, etc.
    virtual void add_operation(const Operation&) {}

    virtual void add_column(const std::string&) {}

    virtual void add_geometry() {}

    /// Method to be called after all `write` has been called
    /// for the last time.
    virtual void finish() {}

    virtual ~OutputWriter() = default;
};

}

#endif // EXACTEXTRACT_OUTPUT_WRITER_H
