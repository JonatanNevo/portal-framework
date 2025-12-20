//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <set>
#include <catch2/catch_test_macros.hpp>

#include "portal/core/jobs/job.h"

namespace portal
{
// Helper functions for job test setup/teardown
inline void job_test_setup()
{
    REQUIRE(JobPromise::get_allocated_size() == 0);
}

inline void job_test_teardown()
{
    REQUIRE(JobPromise::get_allocated_size() == 0);
}

template <class Rep, class Period>
void simulate_work(const std::chrono::duration<Rep, Period>& duration)
{
    const auto start = std::chrono::high_resolution_clock::now();
    int i = 1;
    while (std::chrono::high_resolution_clock::now() - start < duration)
    {
        // Simulate some work
        std::ignore = std::sqrt(++i);
    }
}

// Thread-safe execution tracker for testing
struct ExecutionTracker
{
    std::mutex mutex;
    std::vector<std::string> execution_order;
    std::set<std::string> executed_coroutines;

    void record(const std::string& coroutine_id)
    {
        std::lock_guard<std::mutex> lock(mutex);
        execution_order.push_back(coroutine_id);
        executed_coroutines.insert(coroutine_id);
    }

    bool was_executed(const std::string& coroutine_id)
    {
        std::lock_guard<std::mutex> lock(mutex);
        return executed_coroutines.contains(coroutine_id);
    }

    size_t execution_count()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return execution_order.size();
    }

    // Check if coroutine A executed before coroutine B
    bool executed_before(const std::string& a, const std::string& b)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it_a = std::find(execution_order.begin(), execution_order.end(), a);
        auto it_b = std::find(execution_order.begin(), execution_order.end(), b);
        return it_a != execution_order.end() && it_b != execution_order.end() && it_a < it_b;
    }
};
}
