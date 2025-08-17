//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <memory_resource>
#include <benchmark/benchmark.h>

static void map_of_string(benchmark::State& state) {
    std::unordered_map<uint64_t, std::string> entries;
    for (auto _ : state) {
        entries.emplace(1, "hello_world_long_long_long_long_string");
        entries.emplace(2, "hello_world_long_long_long_long_string");
        entries.emplace(3, "hello_world_long_long_long_long_string");
        entries.emplace(4, "hello_world_long_long_long_long_string");
        entries.emplace(5, "hello_world_long_long_long_long_string");
        entries.emplace(6, "hello_world_long_long_long_long_string");
        entries.emplace(7, "hello_world_long_long_long_long_string");
        entries.emplace(8, "hello_world_long_long_long_long_string");
        entries.emplace(9, "hello_world_long_long_long_long_string");
        entries.emplace(10, "hello_world_long_long_long_long_string");
        entries.emplace(11, "hello_world_long_long_long_long_string");
        entries.emplace(12, "hello_world_long_long_long_long_string");
        entries.emplace(13, "hello_world_long_long_long_long_string");
        entries.emplace(14, "hello_world_long_long_long_long_string");
        entries.emplace(15, "hello_world_long_long_long_long_string");
        entries.emplace(16, "hello_world_long_long_long_long_string");
        entries.emplace(17, "hello_world_long_long_long_long_string");
        entries.emplace(18, "hello_world_long_long_long_long_string");
        entries.emplace(19, "hello_world_long_long_long_long_string");
        entries.emplace(20, "hello_world_long_long_long_long_string");
        entries.emplace(21, "hello_world_long_long_long_long_string");
        entries.emplace(22, "hello_world_long_long_long_long_string");
        entries.emplace(23, "hello_world_long_long_long_long_string");
        entries.emplace(24, "hello_world_long_long_long_long_string");
    }
}
// Register the function as a benchmark
BENCHMARK(map_of_string);

static void pmr_map_of_pmr_string_all(benchmark::State& state) {
    // Code before the loop is not measured
    auto buffer_resource = std::pmr::monotonic_buffer_resource {64 * 1024, std::pmr::new_delete_resource() };
    auto pool_resource = std::pmr::unsynchronized_pool_resource{&buffer_resource};
    std::pmr::unordered_map<uint64_t, std::pmr::string> entries {&pool_resource};
    for (auto _ : state) {
        entries.emplace(1, "hello_world_long_long_long_long_string");
        entries.emplace(2, "hello_world_long_long_long_long_string");
        entries.emplace(3, "hello_world_long_long_long_long_string");
        entries.emplace(4, "hello_world_long_long_long_long_string");
        entries.emplace(5, "hello_world_long_long_long_long_string");
        entries.emplace(6, "hello_world_long_long_long_long_string");
        entries.emplace(7, "hello_world_long_long_long_long_string");
        entries.emplace(8, "hello_world_long_long_long_long_string");
        entries.emplace(9, "hello_world_long_long_long_long_string");
        entries.emplace(10, "hello_world_long_long_long_long_string");
        entries.emplace(11, "hello_world_long_long_long_long_string");
        entries.emplace(12, "hello_world_long_long_long_long_string");
        entries.emplace(13, "hello_world_long_long_long_long_string");
        entries.emplace(14, "hello_world_long_long_long_long_string");
        entries.emplace(15, "hello_world_long_long_long_long_string");
        entries.emplace(16, "hello_world_long_long_long_long_string");
        entries.emplace(17, "hello_world_long_long_long_long_string");
        entries.emplace(18, "hello_world_long_long_long_long_string");
        entries.emplace(19, "hello_world_long_long_long_long_string");
        entries.emplace(20, "hello_world_long_long_long_long_string");
        entries.emplace(21, "hello_world_long_long_long_long_string");
        entries.emplace(22, "hello_world_long_long_long_long_string");
        entries.emplace(23, "hello_world_long_long_long_long_string");
        entries.emplace(24, "hello_world_long_long_long_long_string");
    }
}
BENCHMARK(pmr_map_of_pmr_string_all);

static void pmr_map_of_pmr_string_all_no_init_size(benchmark::State& state) {
    // Code before the loop is not measured
    auto buffer_resource = std::pmr::monotonic_buffer_resource {std::pmr::new_delete_resource() };
    auto pool_resource = std::pmr::unsynchronized_pool_resource{&buffer_resource};

    std::pmr::unordered_map<uint64_t, std::pmr::string> entries {&pool_resource};
    for (auto _ : state)     for (auto _ : state) {
        entries.emplace(1, "hello_world_long_long_long_long_string");
        entries.emplace(2, "hello_world_long_long_long_long_string");
        entries.emplace(3, "hello_world_long_long_long_long_string");
        entries.emplace(4, "hello_world_long_long_long_long_string");
        entries.emplace(5, "hello_world_long_long_long_long_string");
        entries.emplace(6, "hello_world_long_long_long_long_string");
        entries.emplace(7, "hello_world_long_long_long_long_string");
        entries.emplace(8, "hello_world_long_long_long_long_string");
        entries.emplace(9, "hello_world_long_long_long_long_string");
        entries.emplace(10, "hello_world_long_long_long_long_string");
        entries.emplace(11, "hello_world_long_long_long_long_string");
        entries.emplace(12, "hello_world_long_long_long_long_string");
        entries.emplace(13, "hello_world_long_long_long_long_string");
        entries.emplace(14, "hello_world_long_long_long_long_string");
        entries.emplace(15, "hello_world_long_long_long_long_string");
        entries.emplace(16, "hello_world_long_long_long_long_string");
        entries.emplace(17, "hello_world_long_long_long_long_string");
        entries.emplace(18, "hello_world_long_long_long_long_string");
        entries.emplace(19, "hello_world_long_long_long_long_string");
        entries.emplace(20, "hello_world_long_long_long_long_string");
        entries.emplace(21, "hello_world_long_long_long_long_string");
        entries.emplace(22, "hello_world_long_long_long_long_string");
        entries.emplace(23, "hello_world_long_long_long_long_string");
        entries.emplace(24, "hello_world_long_long_long_long_string");
    }
}
BENCHMARK(pmr_map_of_pmr_string_all_no_init_size);

static void pmr_map_of_pmr_string_only_buffer(benchmark::State& state) {
    // Code before the loop is not measured
    auto buffer_resource = std::pmr::monotonic_buffer_resource { 64 * 1024, std::pmr::new_delete_resource() };

    std::pmr::unordered_map<uint64_t, std::pmr::string> entries {&buffer_resource};
    for (auto _ : state) {
        entries.emplace(1, "hello_world_long_long_long_long_string");
        entries.emplace(2, "hello_world_long_long_long_long_string");
        entries.emplace(3, "hello_world_long_long_long_long_string");
        entries.emplace(4, "hello_world_long_long_long_long_string");
        entries.emplace(5, "hello_world_long_long_long_long_string");
        entries.emplace(6, "hello_world_long_long_long_long_string");
        entries.emplace(7, "hello_world_long_long_long_long_string");
        entries.emplace(8, "hello_world_long_long_long_long_string");
        entries.emplace(9, "hello_world_long_long_long_long_string");
        entries.emplace(10, "hello_world_long_long_long_long_string");
        entries.emplace(11, "hello_world_long_long_long_long_string");
        entries.emplace(12, "hello_world_long_long_long_long_string");
        entries.emplace(13, "hello_world_long_long_long_long_string");
        entries.emplace(14, "hello_world_long_long_long_long_string");
        entries.emplace(15, "hello_world_long_long_long_long_string");
        entries.emplace(16, "hello_world_long_long_long_long_string");
        entries.emplace(17, "hello_world_long_long_long_long_string");
        entries.emplace(18, "hello_world_long_long_long_long_string");
        entries.emplace(19, "hello_world_long_long_long_long_string");
        entries.emplace(20, "hello_world_long_long_long_long_string");
        entries.emplace(21, "hello_world_long_long_long_long_string");
        entries.emplace(22, "hello_world_long_long_long_long_string");
        entries.emplace(23, "hello_world_long_long_long_long_string");
        entries.emplace(24, "hello_world_long_long_long_long_string");
    }
}
BENCHMARK(pmr_map_of_pmr_string_only_buffer);

static void pmr_map_of_pmr_string_only_pool(benchmark::State& state) {
    // Code before the loop is not measured
    auto pool_resource = std::pmr::unsynchronized_pool_resource{};

    std::pmr::unordered_map<uint64_t, std::pmr::string> entries {&pool_resource};
    for (auto _ : state)     for (auto _ : state) {
        entries.emplace(1, "hello_world_long_long_long_long_string");
        entries.emplace(2, "hello_world_long_long_long_long_string");
        entries.emplace(3, "hello_world_long_long_long_long_string");
        entries.emplace(4, "hello_world_long_long_long_long_string");
        entries.emplace(5, "hello_world_long_long_long_long_string");
        entries.emplace(6, "hello_world_long_long_long_long_string");
        entries.emplace(7, "hello_world_long_long_long_long_string");
        entries.emplace(8, "hello_world_long_long_long_long_string");
        entries.emplace(9, "hello_world_long_long_long_long_string");
        entries.emplace(10, "hello_world_long_long_long_long_string");
        entries.emplace(11, "hello_world_long_long_long_long_string");
        entries.emplace(12, "hello_world_long_long_long_long_string");
        entries.emplace(13, "hello_world_long_long_long_long_string");
        entries.emplace(14, "hello_world_long_long_long_long_string");
        entries.emplace(15, "hello_world_long_long_long_long_string");
        entries.emplace(16, "hello_world_long_long_long_long_string");
        entries.emplace(17, "hello_world_long_long_long_long_string");
        entries.emplace(18, "hello_world_long_long_long_long_string");
        entries.emplace(19, "hello_world_long_long_long_long_string");
        entries.emplace(20, "hello_world_long_long_long_long_string");
        entries.emplace(21, "hello_world_long_long_long_long_string");
        entries.emplace(22, "hello_world_long_long_long_long_string");
        entries.emplace(23, "hello_world_long_long_long_long_string");
        entries.emplace(24, "hello_world_long_long_long_long_string");
    }
}
BENCHMARK(pmr_map_of_pmr_string_only_pool);

BENCHMARK_MAIN();