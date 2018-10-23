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

#include "csv_utils.h"

namespace exactextract {

    constexpr auto NA = "NA";

    void write_csv_header(const std::string & field_name, const std::vector<std::string> & stats, size_t num_weights, std::ostream & csvout) {
        csvout << field_name;

        for (size_t i = 0; i < std::max(1ul, num_weights); i++) {
            for (auto stat : stats) {
                std::replace(stat.begin(), stat.end(), ' ', '_');
                csvout << "," << stat;

                if (num_weights > 1) {
                    csvout << "_" << (i+1);
                }
            }
        }

        csvout << std::endl;
    }

    void write_nas_to_csv(const std::string & name, size_t n, std::ostream & csvout) {
        csvout << name;

        for (size_t i = 0; i < n; i++) {
            csvout << ',' << NA;
        }

        csvout << std::endl;
    }

    void write_stats_to_csv(const std::string & name, const std::vector<RasterStats<double>> & raster_stats, const std::vector<std::string> & stats, std::ostream & csvout) {
        csvout << name;
        for (const auto& rs : raster_stats) {
            for (const auto& stat : stats) {
                csvout << ",";
                write_stat_to_csv(rs, stat, csvout);
            }
        }
        csvout << std::endl;
    }

    void write_stats_to_csv(const std::string & name, const RasterStats<double> & raster_stats, const std::vector<std::string> & stats, std::ostream & csvout) {
        csvout << name;
        for (const auto& stat : stats) {
            csvout << ",";
            write_stat_to_csv(raster_stats, stat, csvout);
        }
        csvout << std::endl;
    }

    void write_stat_to_csv(const RasterStats<double> & raster_stats, const std::string & stat, std::ostream & csvout) {
        if (stat == "mean") {
            csvout << raster_stats.mean();
        } else if (stat == "count") {
            csvout << raster_stats.count();
        } else if (stat == "sum") {
            csvout << raster_stats.sum();
        } else if (stat == "variety") {
            csvout << raster_stats.variety();
        } else if (stat == "weighted mean") {
            csvout << raster_stats.weighted_mean();
        } else if (stat == "weighted count") {
            csvout << raster_stats.weighted_count();
        } else if (stat == "weighted sum") {
            csvout << raster_stats.weighted_sum();
        } else if (stat == "weighted fraction") {
            if (raster_stats.sum() > 0) {
                csvout << raster_stats.weighted_fraction();
            } else {
                csvout << NA;
            }
        } else if (stat == "min") {
            if (raster_stats.count() > 0) {
                csvout << raster_stats.min();
            } else {
                csvout << NA;
            }
        } else if (stat == "max") {
            if (raster_stats.count() > 0) {
                csvout << raster_stats.max();
            } else {
                csvout << NA;
            }
        } else if (stat == "mode") {
            if (raster_stats.count() > 0) {
                csvout << raster_stats.mode();
            } else {
                csvout << NA;
            }
        } else if (stat == "minority") {
            if (raster_stats.count() > 0) {
                csvout << raster_stats.minority();
            } else {
                csvout << NA;
            }
        } else {
            throw std::runtime_error("Unknown stat: " + stat);
        }
    }

}

