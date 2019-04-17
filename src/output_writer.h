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

#ifndef EXACTEXTRACT_OUTPUT_WRITER_H
#define EXACTEXTRACT_OUTPUT_WRITER_H

#include <string>
#include <vector>

namespace exactextract {

    class Operation;
    class StatsRegistry;

    class OutputWriter {
    public:
        virtual void write(const std::string & fid) = 0;
        virtual void add_operation(const Operation & op) = 0;
        virtual void set_registry(const StatsRegistry* reg) = 0;

        virtual void finish() {};

        virtual ~OutputWriter()= default;

        std::vector<const Operation*> m_ops;
    };

}

#endif //EXACTEXTRACT_OUTPUT_WRITER_H
