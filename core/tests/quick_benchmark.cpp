//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <benchmark/benchmark.h>
#include "portal/core/random/random.h"
#include "portal/core/random/mesmer_twisted_random.h"

static void random_uint(benchmark::State& state)
{
    portal::MesmerTwistedRandom random;
    random.init();
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(random.get_uint());
    }
}

static void random_float(benchmark::State& state)
{
    portal::MesmerTwistedRandom random;
    random.init();

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(random.get_float());
    }
}

static void random_vec3(benchmark::State& state)
{
    portal::MesmerTwistedRandom random;
    random.init();

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(random.get_vec3());
    }
}


BENCHMARK(random_uint);
BENCHMARK(random_float);
BENCHMARK(random_vec3);

BENCHMARK_MAIN();
