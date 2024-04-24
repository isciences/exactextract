// Copyright (c) 2018-2024 ISciences, LLC.
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

#include "operation.h"
#include "utils.h"

#include <charconv>
#include <sstream>

namespace exactextract {

// https://stackoverflow.com/a/62757885/2171894
template<typename>
constexpr bool is_optional_impl = false;
template<typename T>
constexpr bool is_optional_impl<std::optional<T>> = true;
template<typename T>
constexpr bool is_optional =
  is_optional_impl<std::remove_cv_t<std::remove_reference_t<T>>>;

template<typename T>
std::string
make_field_name(const std::string& prefix, const T& value)
{
    if constexpr (std::is_floating_point_v<T>) {
        std::stringstream ss;
        ss << prefix << value;
        return ss.str();
    } else {
        return prefix + std::to_string(value);
    }
}

std::optional<double>
read(const std::string& value)
{
    char* end = nullptr;
    double d = std::strtod(value.data(), &end);
    if (end == value.data() + value.size()) {
        return d;
    }

    return std::nullopt;
}

template<typename T>
T
extract_arg(Operation::ArgMap& options, const std::string& name)
{
    auto it = options.find(name);
    if (it == options.end()) {
        throw std::runtime_error("Missing required argument: " + name);
    }

    const std::string& raw_value = it->second;

    // std::from_chars not supported in clang
    auto parsed = read(raw_value);
    if (!parsed.has_value()) {
        throw std::runtime_error("Failed to parse value of argument: " + name);
    }

    options.erase(it);
    return parsed.value();
}

/// The OperationImpl classes uses CRTP to make it easier to implement the `Operation` interface.
/// In particular, `Operation::set_result` requires some dispatch on `RasterStatsVariant` which
/// would be cumbersome to re-implement for every `Operation`. Instead, an implementation
/// deriving from OperationImpl can implement the following template:
/// template<typename Stats>
/// auto get(const Stats& stats) const
///
/// Macros (REQ_VARIANCE, REQ_VALUES, etc.) are provided to overload Operation methods that
/// control the behavior of `RasterStats`.
///
/// For simple operations, an `OPERATION` macro is provided that allows the implementation
/// class to be defined with a single expression.
template<typename Derived>
class OperationImpl : public Operation
{
  public:
    OperationImpl(std::string p_stat,
                  std::string p_name,
                  RasterSource* p_values,
                  RasterSource* p_weights,
                  ArgMap options)
      : Operation(p_stat, "", p_values, p_weights)
    {
        static_cast<Derived*>(this)->handle_options(options);

        if (!options.empty()) {
            throw std::runtime_error("Unexpected argument(s) to stat: " + stat);
        }

        static_cast<Derived*>(this)->set_name(p_name);
    }

    void handle_options(ArgMap&) {}

    void set_name(const std::string& p_name)
    {
        name = p_name;
    }

    std::unique_ptr<Operation> clone() const override
    {
        return std::make_unique<Derived>(static_cast<const Derived&>(*this));
    }

    Feature::ValueType result_type() const override
    {
        const auto& rast = values->read_empty();

        return std::visit([this](const auto& r) -> Feature::ValueType {
            using value_type = typename std::remove_reference_t<decltype(*r)>::value_type;

            // Determine the type returned by the get() method in the Operation implementation
            // class. The std::declval version of this doesn't work in gcc 9, 10
            // (see https://github.com/isciences/exactextract/issues/82)
#if __GNUC__ < 11
            RasterStats<value_type> stats;
            auto result = static_cast<const Derived*>(this)->get(stats);
            using result_type = std::decay_t<decltype(result)>;
#else
            using result_type = std::decay_t<decltype(static_cast<const Derived*>(this)->get(std::declval<RasterStats<value_type>>()))>;
#endif

            if constexpr (is_optional<result_type>) {
                using inner_result_type = decltype(*std::declval<result_type>());

                if constexpr (std::is_floating_point_v<std::decay_t<inner_result_type>>) {
                    return Feature::ValueType::DOUBLE;
                }

                if constexpr (std::is_integral_v<std::decay_t<inner_result_type>>) {
                    if constexpr (std::is_same_v<std::decay_t<inner_result_type>, std::int64_t> ||
                                  std::is_same_v<std::decay_t<inner_result_type>, std::size_t>) {
                        return Feature::ValueType::INT64;
                    } else {
                        return Feature::ValueType::INT;
                    }
                }
            }

            if constexpr (std::is_floating_point_v<result_type>) {
                return Feature::ValueType::DOUBLE;
            }

            if constexpr (std::is_integral_v<result_type>) {
                if constexpr (std::is_same_v<result_type, std::int64_t> ||
                              std::is_same_v<result_type, std::size_t>) {
                    return Feature::ValueType::INT64;
                } else {
                    return Feature::ValueType::INT;
                }
            }

            if constexpr (std::is_same_v<result_type, std::vector<float>>) {
                return Feature::ValueType::DOUBLE_ARRAY;
            }

            if constexpr (std::is_same_v<result_type, std::vector<double>>) {
                return Feature::ValueType::DOUBLE_ARRAY;
            }

            if constexpr (std::is_same_v<result_type, std::vector<std::int8_t>>) {
                return Feature::ValueType::INT_ARRAY;
            }

            if constexpr (std::is_same_v<result_type, std::vector<std::uint8_t>>) {
                return Feature::ValueType::INT_ARRAY;
            }

            if constexpr (std::is_same_v<result_type, std::vector<std::int16_t>>) {
                return Feature::ValueType::INT_ARRAY;
            }

            if constexpr (std::is_same_v<result_type, std::vector<std::uint16_t>>) {
                return Feature::ValueType::INT_ARRAY;
            }

            if constexpr (std::is_same_v<result_type, std::vector<std::int32_t>>) {
                return Feature::ValueType::INT_ARRAY;
            }

            if constexpr (std::is_same_v<result_type, std::vector<std::uint32_t>>) {
                return Feature::ValueType::INT64_ARRAY;
            }

            if constexpr (std::is_same_v<result_type, std::vector<std::int64_t>>) {
                return Feature::ValueType::INT64_ARRAY;
            }

            if constexpr (std::is_same_v<result_type, std::vector<std::size_t>>) {
                return Feature::ValueType::INT64_ARRAY;
            }

            throw std::runtime_error("Unhandled type in Operation::result_type.");
        },
                          rast);
    }

    void
    set_result(const StatsRegistry::RasterStatsVariant& stats, Feature& f_out) const override
    {
        std::visit([this, &f_out](const auto& s) {
            auto&& value = static_cast<const Derived*>(this)->get(s);

            if constexpr (is_optional<decltype(value)>) {
                std::visit([this, &f_out, &value](const auto& m) {
                    f_out.set(name, value.value_or(m));
                },
                           m_missing);
            } else {
                f_out.set(name, value);
            }
        },
                   stats);
    }
};

#define REQ_HISTOGRAM                        \
    bool requires_histogram() const override \
    {                                        \
        return true;                         \
    }
#define REQ_VARIANCE                        \
    bool requires_variance() const override \
    {                                       \
        return true;                        \
    }
#define REQ_STORED_VALUES                        \
    bool requires_stored_values() const override \
    {                                            \
        return true;                             \
    }
#define REQ_STORED_WEIGHTS                        \
    bool requires_stored_weights() const override \
    {                                             \
        return true;                              \
    }
#define REQ_STORED_COV                                       \
    bool requires_stored_coverage_fractions() const override \
    {                                                        \
        return true;                                         \
    }
#define REQ_STORED_XY                               \
    bool requires_stored_locations() const override \
    {                                               \
        return true;                                \
    }

#define OPERATION(STAT, IMPL, ...)                \
    class STAT : public OperationImpl<STAT>       \
    {                                             \
      public:                                     \
        using OperationImpl<STAT>::OperationImpl; \
        template<typename Stats>                  \
        auto get(const Stats& stats) const        \
        {                                         \
            return (IMPL);                        \
        }                                         \
        __VA_ARGS__                               \
    }

constexpr std::pair<double, double> NAN_PAIR{ std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN() };

OPERATION(CENTER_X, stats.center_x(), REQ_STORED_XY);
OPERATION(CENTER_Y, stats.center_y(), REQ_STORED_XY);
OPERATION(COUNT, stats.count());
OPERATION(COV, stats.coefficient_of_variation(), REQ_VARIANCE);
OPERATION(COVERAGE, stats.coverage_fractions(), REQ_STORED_COV);
OPERATION(MAJORITY, stats.mode(), REQ_HISTOGRAM);
OPERATION(MAX, stats.max());
OPERATION(MAX_CENTER_X, stats.max_xy().value_or(NAN_PAIR).first, REQ_STORED_XY);
OPERATION(MAX_CENTER_Y, stats.max_xy().value_or(NAN_PAIR).second, REQ_STORED_XY);
OPERATION(MEAN, stats.mean());
OPERATION(MEDIAN, stats.quantile(0.5), REQ_HISTOGRAM);
OPERATION(MIN, stats.min());
OPERATION(MINORITY, stats.minority(), REQ_HISTOGRAM);
OPERATION(MIN_CENTER_X, stats.min_xy().value_or(NAN_PAIR).first, REQ_STORED_XY);
OPERATION(MIN_CENTER_Y, stats.min_xy().value_or(NAN_PAIR).second, REQ_STORED_XY);
OPERATION(STDEV, stats.stdev(), REQ_VARIANCE);
OPERATION(SUM, stats.sum());
OPERATION(VALUES, stats.values(), REQ_STORED_VALUES);
OPERATION(VARIANCE, stats.variance(), REQ_VARIANCE);
OPERATION(VARIETY, stats.variety(), REQ_HISTOGRAM);
OPERATION(WEIGHTED_MEAN, stats.weighted_mean());
OPERATION(WEIGHTED_STDEV, stats.weighted_stdev(), REQ_VARIANCE);
OPERATION(WEIGHTED_SUM, stats.weighted_sum());
OPERATION(WEIGHTED_VARIANCE, stats.weighted_variance(), REQ_VARIANCE);
OPERATION(WEIGHTS, stats.weights(), REQ_STORED_WEIGHTS);

class CellId : public OperationImpl<CellId>
{
  public:
    REQ_STORED_XY
    using OperationImpl::OperationImpl;

    template<typename Stats>
    auto get(const Stats& stats) const
    {
        const auto& x = stats.center_x();
        const auto& y = stats.center_y();
        std::vector<std::int64_t> cells(x.size());
        for (std::size_t i = 0; i < x.size(); i++) {
            cells[i] = static_cast<std::int64_t>(values->grid().get_cell(x[i], y[i]));
        }

        return cells;
    }
};

class Quantile : public OperationImpl<Quantile>
{
  public:
    REQ_HISTOGRAM

    using OperationImpl::OperationImpl;

    void handle_options(ArgMap& options)
    {
        m_quantile = extract_arg<double>(options, "q");
        if (m_quantile < 0 || m_quantile > 1) {
            throw std::invalid_argument("Quantile must be between 0 and 1.");
        }
    }

    void set_name(const std::string& p_name)
    {
        name = make_field_name(p_name + "_", static_cast<int>(m_quantile * 100));
    }

    template<typename Stats>
    auto get(const Stats& stats) const
    {
        return stats.quantile(m_quantile);
    }

  private:
    double m_quantile;
};

class Unique : public OperationImpl<Unique>
{
  public:
    REQ_HISTOGRAM
    using OperationImpl::OperationImpl;

    template<typename Stats>
    auto get(const Stats& stats) const
    {
        return std::vector<typename Stats::ValueType>(stats.begin(), stats.end());
    }
};

template<bool Weighted>
class Frac : public OperationImpl<Frac<Weighted>>
{
  public:
    REQ_HISTOGRAM
    using OperationImpl<Frac<Weighted>>::OperationImpl;

    template<typename Stats>
    auto get(const Stats& stats) const
    {
        std::vector<double> fracs;
        fracs.reserve(stats.variety());
        for (const auto& value : stats) {
            if constexpr (Weighted) {
                fracs.push_back(static_cast<double>(stats.weighted_frac(value).value()));
            } else {
                fracs.push_back(static_cast<double>(stats.frac(value).value()));
            }
        }
        return fracs;
    }
};

Operation::
  Operation(std::string p_stat,
            std::string p_name,
            RasterSource* p_values,
            RasterSource* p_weights)
  : stat{ std::move(p_stat) }
  , name{ std::move(p_name) }
  , values{ p_values }
  , weights{ p_weights }
  , m_missing{ get_missing_value() }
{
    if (starts_with(stat, "weighted") && weights == nullptr) {
        throw std::runtime_error("No weights provided for weighted stat: " + stat);
    }

    if (weighted()) {
        m_key = values->name() + "|" + weights->name();
    } else {
        m_key = values->name();
    }
}

bool
Operation::intersects(const Box& box) const
{
    if (!values->grid().extent().intersects(box)) {
        return false;
    }

    if (weighted() && !weights->grid().extent().intersects(box)) {
        return false;
    }

    return true;
}

Grid<bounded_extent>
Operation::grid() const
{
    if (weighted()) {
        return values->grid().common_grid(weights->grid());
    } else {
        return values->grid();
    }
}

std::unique_ptr<Operation>
Operation::create(std::string stat,
                  std::string p_name,
                  RasterSource* p_values,
                  RasterSource* p_weights,
                  std::map<std::string, std::string> options)
{
#define CONSTRUCT(TYP) return std::make_unique<TYP>(stat, p_name, p_values, p_weights, std::move(options))

    if (stat == "cell_id") {
        CONSTRUCT(CellId);
    }

    if (stat == "center_x") {
        CONSTRUCT(CENTER_X);
    }

    if (stat == "center_y") {
        CONSTRUCT(CENTER_Y);
    }

    if (stat == "coefficient_of_variation") {
        CONSTRUCT(COV);
    }
    if (stat == "count") {
        CONSTRUCT(COUNT);
    }
    if (stat == "coverage") {
        CONSTRUCT(COVERAGE);
    }
    if (stat == "frac") {
        CONSTRUCT(Frac<false>);
    }
    if (stat == "majority" || stat == "mode") {
        CONSTRUCT(MAJORITY);
    }
    if (stat == "max") {
        CONSTRUCT(MAX);
    }
    if (stat == "max_center_x") {
        CONSTRUCT(MAX_CENTER_X);
    }
    if (stat == "max_center_y") {
        CONSTRUCT(MAX_CENTER_Y);
    }
    if (stat == "mean") {
        CONSTRUCT(MEAN);
    }
    if (stat == "median") {
        CONSTRUCT(MEDIAN);
    }
    if (stat == "min") {
        CONSTRUCT(MIN);
    }
    if (stat == "minority") {
        CONSTRUCT(MINORITY);
    }
    if (stat == "min_center_x") {
        CONSTRUCT(MIN_CENTER_X);
    }
    if (stat == "min_center_y") {
        CONSTRUCT(MIN_CENTER_Y);
    }
    if (stat == "quantile") {
        CONSTRUCT(Quantile);
    }
    if (stat == "stdev") {
        CONSTRUCT(STDEV);
    }
    if (stat == "sum") {
        CONSTRUCT(SUM);
    }
    if (stat == "unique") {
        CONSTRUCT(Unique);
    }
    if (stat == "values") {
        CONSTRUCT(VALUES);
    }
    if (stat == "variance") {
        CONSTRUCT(VARIANCE);
    }
    if (stat == "variety") {
        CONSTRUCT(VARIETY);
    }
    if (stat == "weighted_frac") {
        CONSTRUCT(Frac<true>);
    }
    if (stat == "weighted_mean") {
        CONSTRUCT(WEIGHTED_MEAN);
    }
    if (stat == "weighted_stdev") {
        CONSTRUCT(WEIGHTED_STDEV);
    }
    if (stat == "weighted_sum") {
        CONSTRUCT(WEIGHTED_SUM);
    }
    if (stat == "weighted_variance") {
        CONSTRUCT(WEIGHTED_VARIANCE);
    }
    if (stat == "weights") {
        CONSTRUCT(WEIGHTS);
    }

    throw std::runtime_error("Unsupported stat: " + stat);
#undef CONSTRUCT
}

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

const StatsRegistry::RasterStatsVariant&
Operation::empty_stats() const
{
    static const StatsRegistry::RasterStatsVariant ret = std::visit([](const auto& r) {
        using value_type = typename std::remove_reference_t<decltype(*r)>::value_type;

        return StatsRegistry::RasterStatsVariant{
            RasterStats<value_type>()
        };
    },
                                                                    values->read_empty());

    return ret;
}

void
Operation::set_result(const StatsRegistry& reg, const Feature& f_in, Feature& f_out) const
{
    constexpr bool write_if_missing = true;
    // should we set attribute values if the feature did not intersect the raster?
    if (!write_if_missing && !reg.contains(f_in, *this)) {
        return;
    }

    const auto& stats = reg.contains(f_in, *this) ? reg.stats(f_in, *this) : empty_stats();

    set_result(stats, f_out);
}

void
Operation::set_empty_result(Feature& f_out) const
{
    set_result(empty_stats(), f_out);
}
}
