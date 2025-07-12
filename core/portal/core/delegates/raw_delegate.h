//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <utility>

#include "delegate_callback.h"

namespace portal
{

template <bool IsConst, typename T, typename RetVal, typename... Args>
class RawDelegate;

template <bool IsConst, typename T, typename RetVal, typename... Args, typename... Payload>
class RawDelegate<IsConst, T, RetVal(Args...), Payload...> final : public DelegateInterface<RetVal, Args...>
{
public:
    using DelegateFunction = typename delegates::MemberFunction<IsConst, T, RetVal, Args..., Payload...>::Type;

    RawDelegate(T* object, DelegateFunction function, Payload&&... payload) :
        object(object), function(function), payload(std::forward<Payload>(payload)...)
    {
    }

    RawDelegate(T* object, DelegateFunction function, const std::tuple<Payload...>& payload) :
        object(object), function(function), payload(payload)
    {
    }

    void clone(void* destination) override
    {
        new(destination) RawDelegate(object, function, payload);
    }

    RetVal execute(Args&&... args) override
    {
        return execute_internal(std::forward<Args>(args)..., std::index_sequence_for<Payload...>());
    }

    [[nodiscard]] const void* get_owner() const override
    {
        return object;
    }

private:
    template <std::size_t... Is>
    RetVal execute_internal(Args&&... args, std::index_sequence<Is...>)
    {
        return (object->*function)(std::forward<Args>(args)..., std::get<Is>(payload)...);
    }

    T* object;
    DelegateFunction function;
    std::tuple<Payload...> payload;
};
}
