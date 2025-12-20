//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <catch2/catch_test_macros.hpp>

#include "portal/core/jobs/basic_coroutine.h"
#include "portal/core/jobs/task.h"

namespace portal
{
TEST_CASE("Task Execution Order", "[task]")
{
    std::vector<std::string> log;

    auto task = [&]() -> Task<std::string>
    {
        log.push_back("task_start");
        co_return "result";
    };

    auto wrapper = [&](Task<std::string> inner) -> Task<>
    {
        log.push_back("wrapper_start");
        std::string result = co_await inner;
        log.push_back(result);
        log.push_back("wrapper_end");
    };

    auto main_coro = [&]() -> Task<>
    {
        log.push_back("main_start");
        co_await wrapper(task());
        log.push_back("main_end");
        co_return;
    };

    execute(main_coro());

    // Verify execution order
    REQUIRE(log.size() == 6);
    REQUIRE(log[0] == "main_start");
    REQUIRE(log[1] == "wrapper_start");
    REQUIRE(log[2] == "task_start");
    REQUIRE(log[3] == "result");
    REQUIRE(log[4] == "wrapper_end");
    REQUIRE(log[5] == "main_end");
}
}
