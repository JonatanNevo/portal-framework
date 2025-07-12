//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <utility>

#include "delegate_callback.h"

namespace portal
{
template <typename RetVal, typename... Args>
class StaticDelegate;

template <typename RetVal, typename... Args, typename... Payload>
class StaticDelegate<RetVal(Args...), Payload...> final : public DelegateInterface<RetVal, Args...>
{
public:
    using DelegateFunction = RetVal(*)(Args..., Payload...);

    StaticDelegate(const DelegateFunction function, Payload&&... payload) :
        function(function), payload(std::forward<Payload>(payload)...)
    {
    }

    StaticDelegate(const DelegateFunction function, const std::tuple<Payload...>& payload) :
        function(function), payload(payload)
    {
    }

    RetVal execute(Args&&... args) override
    {
        return execute_internal(std::forward<Args>(args)..., std::index_sequence_for<Payload...>());
    }

    void clone(void* destination) override
    {
        new(destination) StaticDelegate(function, payload);
    }

private:
    template <std::size_t... Is>
    RetVal execute_internal(Args&&... args, std::index_sequence<Is...>)
    {
        return function(std::forward<Args>(args)..., std::get<Is>(payload)...);
    }

    DelegateFunction function;
    std::tuple<Payload...> payload;
};
}
