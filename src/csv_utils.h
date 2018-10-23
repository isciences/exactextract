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

#ifndef EXACTEXTRACT_CSV_UTILS_H
#define EXACTEXTRACT_CSV_UTILS_H

#include "raster_stats.h"

namespace exactextract {

    void write_stat_to_csv(
            const RasterStats<double> & raster_stats,
            const std::string & stat,
            std::ostream & csvout);

    void write_stats_to_csv(
            const std::string & name,
            const RasterStats<double> & raster_stats,
            const std::vector<std::string> & stats,
            std::ostream & csvout);

    void write_stats_to_csv(
            const std::string & name,
            const std::vector<RasterStats<double>> & raster_stats,
            const std::vector<std::string> & stats,
            std::ostream & csvout);

    void write_csv_header(
            const std::string & field_name,
            const std::vector<std::string> & stats,
            size_t num_weights,
            std::ostream & csvout);

}



#endif //EXACTEXTRACT_CSV_UTILS_H
