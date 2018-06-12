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

#include <ostream>

#include "side.h"

namespace exactextract {

    std::ostream &operator<<(std::ostream &os, const Side &s) {
        switch (s) {
            case Side::NONE:
                os << "none";
                return os;
            case Side::LEFT:
                os << "left";
                return os;
            case Side::RIGHT:
                os << "right";
                return os;
            case Side::TOP:
                os << "top";
                return os;
            case Side::BOTTOM:
                os << "bottom";
                return os;
        }

        return os;
    }

}