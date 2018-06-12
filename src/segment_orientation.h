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

#ifndef RASTER_OVERLAY_CPP_SEGMENT_ORIENTATION_H
#define RASTER_OVERLAY_CPP_SEGMENT_ORIENTATION_H

namespace exactextract {

    enum class SegmentOrientation {
        HORIZONTAL_RIGHT,
        HORIZONTAL_LEFT,
        VERTICAL_UP,
        VERTICAL_DOWN,
        ANGLED
    };

}

#endif //RASTER_OVERLAY_CPP_SEGMENT_ORIENTATION_H
