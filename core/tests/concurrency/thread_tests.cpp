//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <atomic>
#include <chrono>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "portal/platform/core/hal/thread.h"

using namespace portal;
using namespace std::chrono_literals;

namespace
{
ThreadSpecification make_spec(std::string name)
{
    ThreadSpecification spec;
    spec.name = std::move(name);
    return spec; // default priority/affinity — avoids CI permission issues
}

// Spin-wait until the flag is set or the deadline passes; returns final flag value.
bool wait_for(const std::atomic<bool>& flag, std::chrono::milliseconds timeout = 2000ms)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!flag.load(std::memory_order_acquire))
    {
        if (std::chrono::steady_clock::now() >= deadline)
            return false;
        std::this_thread::yield();
    }
    return true;
}
}

TEST_CASE("Thread runs a plain task and forwards arguments", "[thread]")
{
    std::atomic<int> sum{0};
    std::atomic<bool> done{false};

    Thread thread(
        make_spec("plain"),
        [&](int a, int b)
        {
            sum.store(a + b, std::memory_order_relaxed);
            done.store(true, std::memory_order_release);
        },
        20,
        22);

    thread.join();

    REQUIRE(done.load());
    REQUIRE(sum.load() == 42);
    REQUIRE_FALSE(thread.joinable());
}

TEST_CASE("Thread reports id and name", "[thread]")
{
    std::atomic<bool> running{false};
    std::atomic<bool> release{false};

    Thread thread(
        make_spec("named-worker"),
        [&](const std::stop_token&)
        {
            running.store(true, std::memory_order_release);
            wait_for(release); // hold the thread alive while we inspect it
        });

    REQUIRE(wait_for(running));
    REQUIRE(thread.joinable());
    REQUIRE(thread.get_id() != std::thread::id{});
    REQUIRE(thread.get_name() == "named-worker");

    release.store(true, std::memory_order_release);
    thread.join();
}

TEST_CASE("Thread request_stop propagates a stop_token", "[thread]")
{
    std::atomic<bool> started{false};
    std::atomic<bool> observed_stop{false};

    Thread thread(
        make_spec("stoppable"),
        [&](const std::stop_token& st)
        {
            started.store(true, std::memory_order_release);
            while (!st.stop_requested())
                std::this_thread::yield();
            observed_stop.store(true, std::memory_order_release);
        });

    REQUIRE(wait_for(started));
    REQUIRE(thread.request_stop());

    thread.join();
    REQUIRE(observed_stop.load());
}

TEST_CASE("Thread move-assignment cancels and joins the previous thread", "[thread]")
{
    std::atomic<bool> first_started{false};
    std::atomic<bool> first_exited{false};

    Thread thread(
        make_spec("first"),
        [&](const std::stop_token& st)
        {
            first_started.store(true, std::memory_order_release);
            while (!st.stop_requested())
                std::this_thread::yield();
            first_exited.store(true, std::memory_order_release);
        });

    REQUIRE(wait_for(first_started));

    std::atomic<bool> second_done{false};
    // move-assign: must stop+join "first" before taking over
    thread = Thread(
        make_spec("second"),
        [&](const std::stop_token&) { second_done.store(true, std::memory_order_release); });

    REQUIRE(first_exited.load()); // guaranteed joined by the time assignment returns
    REQUIRE(thread.get_name() == "second");

    thread.join();
    REQUIRE(second_done.load());
}

TEST_CASE("Thread destructor cancels and joins (RAII)", "[thread]")
{
    std::atomic<bool> started{false};
    std::atomic<bool> exited{false};

    {
        Thread thread(
            make_spec("scoped"),
            [&](const std::stop_token& st)
            {
                started.store(true, std::memory_order_release);
                while (!st.stop_requested())
                    std::this_thread::yield();
                exited.store(true, std::memory_order_release);
            });

        REQUIRE(wait_for(started));
    } // destructor: request_stop + join

    REQUIRE(exited.load());
}

TEST_CASE("Thread can be detached", "[thread]")
{
    std::atomic<bool> done{false};

    Thread thread(
        make_spec("detached"),
        [&](const std::stop_token&) { done.store(true, std::memory_order_release); });

    thread.detach();
    REQUIRE_FALSE(thread.joinable());
    REQUIRE(wait_for(done)); // task still completes after detach
}