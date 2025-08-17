#pragma once

#include <memory>


#include <thread>
#include <functional>
#include <type_traits>

#include "portal/core/debug/profile.h"

namespace portal
{


/**
 * Named thread with an interface similar to jthread
 */
class Thread
{
public:
    Thread() = default;

    template <typename F, typename... Args>
    explicit Thread(const std::string name, F&& f, Args&&... args): name(name)
    {
        auto callable = make_callable(std::forward<F>(f), std::forward<Args>(args)...);
        thread = std::jthread([callable = std::move(callable), c_name = name.c_str()](std::stop_token st) mutable
        {
            PORTAL_NAME_THREAD(c_name);
            callable(st);
        });
    }

    ~Thread()
    {
        try_cancel_and_join();
    }

    Thread(const Thread&) = delete;
    Thread(Thread&&) noexcept = default;
    Thread& operator=(const Thread&) = delete;

    Thread& operator=(Thread&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        try_cancel_and_join();
        thread = std::move(other.thread);
        name = std::move(other.name);
        return *this;
    }

    [[nodiscard]] bool joinable() const noexcept
    {
        return thread.joinable();
    }

    void join()
    {
        thread.join();
    }

    void detach()
    {
        thread.detach();
    }

    [[nodiscard]] std::thread::id get_id() const noexcept
    {
        return thread.get_id();
    }

    [[nodiscard]] std::string_view get_name() const noexcept
    {
        return name;
    }

    bool request_stop() noexcept
    {
        return thread.request_stop();
    }

protected:
    template <typename F, typename... Args>
    static auto make_callable(F&& f, Args&&... args)
    {
        if constexpr (std::is_invocable_v<std::decay_t<F>, std::stop_token, std::decay_t<Args>...>)
        {
            return [f = std::forward<F>(f), args...](std::stop_token st) mutable
            {
                return std::invoke(f, st, args...);
            };
        }
        else
        {
            return [f = std::forward<F>(f), args...](std::stop_token) mutable
            {
                return std::invoke(f, args...);
            };
        }
    }

    void try_cancel_and_join()
    {
        if (thread.joinable())
        {
            thread.request_stop();
            thread.join();
        }
    }

private:
    std::string name{};
    std::jthread thread;
};
}
