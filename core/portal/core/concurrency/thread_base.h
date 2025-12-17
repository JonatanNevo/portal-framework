#pragma once

#include <thread>
#include <functional>
#include <string>
#include <type_traits>

namespace portal
{
enum class ThreadPriority
{
    Low,
    Default,
    High,
};

enum class ThreadAffinity
{
    Default,
    Core,
    CoreLean,
};

struct ThreadSpecification
{
    // TODO: change to string_id
    std::string name;
    ThreadPriority priority = ThreadPriority::Default;
    ThreadAffinity affinity = ThreadAffinity::Default;
    uint16_t core = 0;
};

/**
 * A base thread object, with simular api to jthread.
 * In order to use this, use the platform specific `portal/core/hal/thread.h` `Thread`
 */
class ThreadBase
{
public:
    ThreadBase() = default;

    explicit ThreadBase(const ThreadSpecification& spec) : spec(spec) {}

    virtual ~ThreadBase();
    ThreadBase(const ThreadBase&) = delete;
    ThreadBase(ThreadBase&&) noexcept = default;

    ThreadBase& operator=(const ThreadBase&) = delete;
    ThreadBase& operator=(ThreadBase&& other) noexcept;

    [[nodiscard]] bool joinable() const noexcept;

    void join();
    void detach();

    [[nodiscard]] std::thread::id get_id() const noexcept;
    [[nodiscard]] std::string get_name() const noexcept;

    bool request_stop() noexcept;

protected:
    template <typename F, typename... Args>
    static auto make_callable(F&& f, Args&&... args)
    {
        if constexpr (std::is_invocable_v<std::decay_t<F>, std::stop_token, Args...>)
        {
            return [
                    func = std::decay_t<F>(std::forward<F>(f)),
                    ...bound = std::tuple<Args&&>(std::forward<Args>(args))
                ](std::stop_token st) mutable
            {
                return std::apply(
                    [&](auto&... t)
                    {
                        return std::invoke(func, st, (std::get<0>(t))...);
                    },
                    std::forward_as_tuple(bound...)
                );
            };
        }
        else
        {
            return [
                    func = std::decay_t<F>(std::forward<F>(f)),
                    ...bound = std::tuple<Args&&>(std::forward<Args>(args))
                ](std::stop_token) mutable
            {
                return std::apply(
                    [&](auto&... t)
                    {
                        return std::invoke(func, (std::get<0>(t))...);
                    },
                    std::forward_as_tuple(bound...)
                );
            };
        }
    }

    void try_cancel_and_join();

    static void set_name(const std::string& name);

protected:
    ThreadSpecification spec{};
    std::jthread thread;
};
}
