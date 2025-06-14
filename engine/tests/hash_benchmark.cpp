//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <benchmark/benchmark.h>

#include "../portal/engine/strings/md5_hash.h"


static void portal_md5_hash(benchmark::State& state)
{
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(portal::hash::md5("hello"));
    }
}

static void std_default_hash(benchmark::State& state)
{
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(std::hash<std::string>()("hello"));
    }
}

static void noop(benchmark::State& state)
{
    for (auto _ : state)
    {
        benchmark::DoNotOptimize("hello");
    }
}

BENCHMARK(noop);
BENCHMARK(portal_md5_hash);
BENCHMARK(std_default_hash);