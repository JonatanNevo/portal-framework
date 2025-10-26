//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <coroutine>

namespace portal
{

/**
 * Basic coroutine, not awaitable and does not have either return value nor parameters
 */
class BasicCoroutine
{
public:
    struct Promise
    {
        static BasicCoroutine get_return_object() { return {}; }

        static void unhandled_exception() noexcept {}
        static void return_void() noexcept {}

        static std::suspend_never initial_suspend() noexcept { return {}; }
        static std::suspend_never final_suspend() noexcept { return {}; }
    };

    using promise_type = Promise;
};

/**
 * Immediately executes an awaitable (like Task)
 *
 * @tparam Awaitable The awaitable type, eg: Task
 * @param awaitable The coroutine
 */
template<typename Awaitable>
BasicCoroutine execute(Awaitable awaitable)
{
    co_await awaitable;
}

}