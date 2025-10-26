//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <gtest/gtest.h>

#include "portal/core/jobs/job.h"

namespace portal
{

class JobTest: public testing::Test
{
protected:
    void SetUp() override
    {
        EXPECT_EQ(JobPromise::get_allocated_size(), 0) << "Allocator has invalid state";
    }

    void TearDown() override
    {
        EXPECT_EQ(JobPromise::get_allocated_size(), 0) << "Memory leak in test";
    }
};

template <class Rep, class Period>
void simulate_work(const std::chrono::duration<Rep, Period>& duration)
{
    [[maybe_unused]] volatile double sink = 0.0;

    const auto start = std::chrono::high_resolution_clock::now();
    int i = 1;
    while (std::chrono::high_resolution_clock::now() - start < duration)
    {
        // Simulate some work
        sink = std::sqrt(++i);
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
