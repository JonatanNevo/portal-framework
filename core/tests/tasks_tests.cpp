//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <gtest/gtest.h>

#include "portal/core/jobs/basic_coroutine.h"
#include "portal/core/jobs/task.h"

namespace portal
{

TEST(TaskTests, BasicExecution)  // Rename from 'sanity'
{
    std::vector<std::string> log;

    auto task = [&]() -> Task<std::string> {
        log.push_back("task_start");
        co_return "result";
    };

    auto wrapper = [&](Task<std::string> inner) -> Task<> {
        log.push_back("wrapper_start");
        std::string result = co_await inner;
        log.push_back(result);
        log.push_back("wrapper_end");
    };

    auto main_coro = [&]() -> Task<> {
        log.push_back("main_start");
        co_await wrapper(task());
        log.push_back("main_end");
        co_return;
    };

    execute(main_coro());

    // Verify execution order
    ASSERT_EQ(log.size(), 6);
    EXPECT_EQ(log[0], "main_start");
    EXPECT_EQ(log[1], "wrapper_start");
    EXPECT_EQ(log[2], "task_start");
    EXPECT_EQ(log[3], "result");
    EXPECT_EQ(log[4], "wrapper_end");
    EXPECT_EQ(log[5], "main_end");
}

}