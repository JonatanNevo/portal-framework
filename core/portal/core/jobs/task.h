//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <coroutine>

#include "portal/core/log.h"

namespace portal
{
template <typename Result = void>
class [[nodiscard]] Task
{
public:
    struct FinalAwaiter
    {
        bool await_ready() noexcept { return false; }

        template <typename P>
        auto await_suspend(std::coroutine_handle<P> handle) noexcept
        {
            return handle.promise().continuation;
        }

        void await_resume() noexcept {}
    };

    struct Promise
    {
        std::coroutine_handle<> continuation;
        Result result;

        Task get_return_object()
        {
            return Task{std::coroutine_handle<Promise>::from_promise(*this)};
        }

        void unhandled_exception() noexcept
        {
            LOG_ERROR_TAG("Task", "Unhandled exception in task");
        }

        void return_value(Result res) noexcept
        {
            result = std::move(res);
        }

        std::suspend_always initial_suspend() noexcept { return {}; }
        FinalAwaiter final_suspend() noexcept { return {}; }
    };

    struct Awaiter
    {
        std::coroutine_handle<Promise> handle;

        bool await_ready() noexcept { return !handle || handle.done(); }

        auto await_suspend(std::coroutine_handle<> calling) noexcept
        {
            handle.promise().continuation = calling;
            return handle;
        }

        template <typename T = Result> requires (std::is_same_v<T, void>)
        void await_resume() noexcept {}

        template <typename T = Result> requires (!std::is_same_v<T, void>)
        T await_resume() noexcept
        {
            return std::move(handle.promise().result);
        }
    };

    using promise_type = Promise;

    Task() = default;
    Task(Task&& other) noexcept : handle(std::exchange(other.handle, nullptr)) {}

    Task& operator=(Task&& other) noexcept
    {
        if (this == &other)
            return *this;

        if (handle)
            handle.destroy();

        handle = std::exchange(other.handle, nullptr);

        return *this;
    }

    ~Task()
    {
        if (handle)
            handle.destroy();
    }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    auto operator co_await() noexcept
    {
        return Awaiter{handle};
    }

private:
    explicit Task(std::coroutine_handle<Promise> handle) : handle(handle) {}
    std::coroutine_handle<Promise> handle;
};

template <>
struct Task<void>::Promise
{
    std::coroutine_handle<> continuation;

    Task<void> get_return_object()
    {
        return Task<void>{std::coroutine_handle<Promise>::from_promise(*this)};
    }

    void unhandled_exception() noexcept
    {
        LOG_ERROR_TAG("Task", "Unhandled exception in task");
    }

    void return_void() noexcept {}

    std::suspend_always initial_suspend() noexcept { return {}; }
    Task<void>::FinalAwaiter final_suspend() noexcept { return {}; }
};
}
