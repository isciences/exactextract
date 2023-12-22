#include "operation.h"

#include <sstream>

namespace exactextract {

Operation::missing_value_t
Operation::get_missing_value()
{

    const auto& empty_rast = values->read_empty();

    return std::visit([](const auto& r) -> missing_value_t {
        if (r->has_nodata()) {
            return r->nodata();
        }
        return std::numeric_limits<double>::quiet_NaN();
    },
                      empty_rast);
}

void
Operation::set_result(const StatsRegistry& reg, const Feature& f_in, Feature& f_out) const
{
    static const StatsRegistry::RasterStatsVariant empty_stats = RasterStats<double>();

    constexpr bool write_if_missing = true; // should we set attribute values if the feature did not intersect the raster?
    if (!write_if_missing && !reg.contains(f_in, *this)) {
        return;
    }

    const auto& stats = reg.contains(f_in, *this) ? reg.stats(f_in, *this) : empty_stats;

    if (stat == "mean") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.mean()); }, stats);
    } else if (stat == "sum") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.sum()); }, stats);
    } else if (stat == "count") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.count()); }, stats);
    } else if (stat == "weighted_mean") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.weighted_mean()); }, stats);
    } else if (stat == "weighted_sum") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.weighted_sum()); }, stats);
    } else if (stat == "min") {
        std::visit([&f_out, this](const auto& x, const auto& m) { f_out.set(m_field_names[0], x.min().value_or(m)); }, stats, m_missing);
    } else if (stat == "max") {
        std::visit([&f_out, this](const auto& x, const auto& m) { f_out.set(m_field_names[0], x.max().value_or(m)); }, stats, m_missing);
    } else if (stat == "majority" || stat == "mode") {
        std::visit([&f_out, this](const auto& x, const auto& m) { f_out.set(m_field_names[0], x.mode().value_or(m)); }, stats, m_missing);
    } else if (stat == "minority") {
        std::visit([&f_out, this](const auto& x, const auto& m) { f_out.set(m_field_names[0], x.minority().value_or(m)); }, stats, m_missing);
    } else if (stat == "variety") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.variety()); }, stats);
    } else if (stat == "stdev") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.stdev()); }, stats);
    } else if (stat == "weighted_stdev") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.weighted_stdev()); }, stats);
    } else if (stat == "variance") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.variance()); }, stats);
    } else if (stat == "weighted_variance") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.weighted_variance()); }, stats);
    } else if (stat == "coefficient_of_variation") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.coefficient_of_variation()); }, stats);
    } else if (stat == "median") {
        std::visit([&f_out, this](const auto& x, const auto& m) { f_out.set(m_field_names[0], x.quantile(0.5).value_or(m)); }, stats, m_missing);
    } else if (stat == "quantile") {
        std::visit([&f_out, this](const auto& x, const auto& m) {
            for (std::size_t i = 0; i < m_quantiles.size(); i++) {
                f_out.set(m_field_names[i], x.quantile(m_quantiles[i]).value_or(m));
            }
        },
                   stats,
                   m_missing);
    } else if (stat == "frac") {
        std::visit([&f_out](const auto& x) {
            for (const auto& value : x) {
                std::stringstream s;
                s << "frac_" << value;

                f_out.set(s.str(), x.frac(value).value_or(0));
            }
        },
                   stats);
    } else if (stat == "weighted_frac") {
        std::visit([&f_out](const auto& x) {
            for (const auto& value : x) {
                std::stringstream s;
                s << "weighted_frac_" << value;

                f_out.set(s.str(), x.weighted_frac(value).value_or(0));
            }
        },
                   stats);
    } else {
        throw std::runtime_error("Unhandled stat: " + stat);
    }
}

void
Operation::setQuantileFieldNames()
{

    for (double q : m_quantiles) {
        std::stringstream ss;
        int qint = static_cast<int>(100 * q);
        ss << "q_" << qint;
        m_field_names.push_back(ss.str());
    }
}

}
