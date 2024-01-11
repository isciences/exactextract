#include <benchmark/benchmark.h>

#include "matrix.h"
#include "raster.h"
#include "raster_stats.h"

#include <random>

using namespace exactextract;

template<typename T>
Raster<T>
random_raster(std::default_random_engine& e, std::size_t nx, std::size_t ny, T min, T max, double frac_nodata)
{
    Matrix<T> values(ny, nx);

    std::bernoulli_distribution nodata_dist(frac_nodata);
    std::uniform_real_distribution<> value_dist(min, max);

    for (std::size_t i = 0; i < ny; i++) {
        for (std::size_t j = 0; j < nx; j++) {
            if (nodata_dist(e)) {
                values(i, j) = std::numeric_limits<T>::quiet_NaN();
            } else {
                values(i, j) = value_dist(e);
            }
        }
    }

    return Raster<T>(std::move(values), Box{ 0, 0, (double)nx, (double)ny });
}

static void
unweighted_stats(benchmark::State& state, RasterStatsOptions opt)
{
    std::default_random_engine e(12345);

    const auto values = random_raster<double>(e, 10000, 10000, 0, 100, 0.02);
    const auto coverage = random_raster<float>(e, 10000, 10000, 0, 1, 0);

    for (auto _ : state) {
        RasterStats<double> stats(opt);
        stats.process(coverage, values);
    }
}

static void
weighted_stats(benchmark::State& state, RasterStatsOptions opt)
{
    std::default_random_engine e(12345);

    const auto values = random_raster<double>(e, 10000, 10000, 0, 100, 0.02);
    const auto weights = random_raster<double>(e, 10000, 10000, 0, 3, 0.02);
    const auto coverage = random_raster<float>(e, 10000, 10000, 0, 1, 0);

    for (auto _ : state) {
        RasterStats<double> stats(opt);
        stats.process(coverage, values, weights);
    }
}

static void
BM_DefaultStatsUnweighted(benchmark::State& state)
{
    unweighted_stats(state, RasterStatsOptions());
}

static void
BM_DefaultStatsWeighted(benchmark::State& state)
{
    weighted_stats(state, RasterStatsOptions());
}

BENCHMARK(BM_DefaultStatsUnweighted);
BENCHMARK(BM_DefaultStatsWeighted);

BENCHMARK_MAIN();