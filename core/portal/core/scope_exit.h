//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <new>
#include <type_traits>
#include <utility>

#include "debug/assert.h"

namespace portal
{
namespace details
{
    struct ScopeExitTag {};

    template <typename Arg, typename... Args>
    constexpr bool was_deduced()
    {
        return std::is_same_v<ScopeExitTag, Arg> && sizeof...(Args) == 0;
    }

    template <typename Callback>
    constexpr bool returns_void()
    {
        return std::is_same_v<std::invoke_result_t<Callback>, void>;
    }

    template <typename Callback>
    class ScopeExitStorage
    {
    public:
        explicit ScopeExitStorage(Callback callback)
        {
            ::new(get_callback_buffer()) Callback(std::move(callback));
            callback_set = true;
        }

        ScopeExitStorage(ScopeExitStorage&& other) noexcept
        {
            PORTAL_ASSERT(other.is_callback_set(), "Moving from ScopeExitStorage that is not set");

            ::new(get_callback_buffer()) Callback(std::move(other.get_callback()));
            callback_set = true;

            other.destroy_callback();
        }

        ScopeExitStorage(ScopeExitStorage& other) = delete;
        ScopeExitStorage& operator=(ScopeExitStorage& other) = delete;
        ScopeExitStorage& operator=(ScopeExitStorage&& other) = delete;

        void* get_callback_buffer() { return static_cast<void*>(callback_buffer); }

        Callback& get_callback()
        {
            return *static_cast<Callback*>(get_callback_buffer());
        }

        [[nodiscard]] bool is_callback_set() const { return callback_set; }

        void destroy_callback()
        {
            get_callback().~Callback();
            callback_set = false;
        }

        void invoke_callback()
        {
            std::move(get_callback())();
        }

    private:
        bool callback_set;
        alignas(Callback) std::byte callback_buffer[sizeof(Callback)]{};
    };
}

/**
 * Implements the scope guard idiom, invoking the contained callback's `operator()() &&` on scope exit.
 *
 * This class doesn't allocate or take any locks and is safe to use in a signal handler.
 * Of course, the callback with which it is constructed also must be signal safe in order for this to be useful.
 * @tparam Callback
 */
template <typename Arg, typename Callback = void()>
class [[nodiscard]] ScopeExit final
{
    static_assert(details::was_deduced<Arg>(), "Explicit template parameters are not supported.");
    static_assert(details::returns_void<Callback>(), "Callbacks that return values are not supported.");

public:
    explicit ScopeExit(Callback callback) : storage(std::move(callback)) {}
    ScopeExit(ScopeExit&& other) noexcept = default;

    ~ScopeExit()
    {
        if (storage.is_callback_set())
        {
            storage.invoke_callback();
            storage.destroy_callback();
        }
    }

    /**
     * Prevents the callback from executing.
     *
     * @example
     * portal::ScopeExit exit([]() { std::cout << "Hello, world!\n"; });
     * std::move(exit).cancel();
     */
    void cancel() &&
    {
        PORTAL_ASSERT(storage.is_callback_set(), "ScopeExit::cancel() called on an already canceled ScopeExit.");
        storage.destroy_callback();
    }

    /**
     * Executes the callback early, before destruction, and prevents the callback from executing in the destructor.
     *
     * @example
     * portal::ScopeExit exit([]() { std::cout << "Hello, world!\n"; });
     * std::move(exit).invoke();
     */
    void invoke() &&
    {
        PORTAL_ASSERT(storage.is_callback_set(), "ScopeExit::invoke() called on an already invoked ScopeExit.");
        storage.invoke_callback();
        storage.destroy_callback();
    }

private:
    details::ScopeExitStorage<Callback> storage;
};

/**
 * Type deduction API for creating instance of `ScopeExit`
 */
template <typename Callback>
ScopeExit(Callback callback) -> ScopeExit<details::ScopeExitTag, Callback>;
}
