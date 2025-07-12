//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <memory>

#include "delegate_callback.h"

namespace portal
{

template <bool IsConst, typename T, typename RetVal, typename... Args>
class SharedPointerDelegate;

template <bool IsConst, typename RetVal, typename T, typename... Args, typename... Payload>
class SharedPointerDelegate<IsConst, T, RetVal(Args...), Payload...> : public DelegateInterface<RetVal, Args...>
{
public:
    using DelegateFunction = typename delegates::MemberFunction<IsConst, T, RetVal, Args..., Payload...>::Type;

    SharedPointerDelegate(std::shared_ptr<T> object, DelegateFunction function, Payload&&... payload) :
        object(object),
        function(function),
        payload(std::forward<Payload>(payload)...)
    {
    }

    SharedPointerDelegate(std::weak_ptr<T> object, DelegateFunction function, const std::tuple<Payload...>& payload) :
        object(object),
        function(function),
        payload(payload)
    {
    }

    const void* get_owner() const override
    {
        return object.expired() ? nullptr : object.lock().get();
    }

    void clone(void* destination) override
    {
        new(destination) SharedPointerDelegate(object, function, payload);
    }

    RetVal execute(Args&&... args) override
    {
        return execute_internal(std::forward<Args>(args)..., std::index_sequence_for<Payload...>());
    }

private:
    template <std::size_t... Is>
    RetVal execute_internal(Args&&... args, std::index_sequence<Is...>)
    {
        // TODO: The ret val might not be trivially constructed
        if (object.expired())
            return RetVal();

        std::shared_ptr<T> pPinned = object.lock();
        return (pPinned.get()->*function)(std::forward<Args>(args)..., std::get<Is>(payload)...);
    }

    std::weak_ptr<T> object;
    DelegateFunction function;
    std::tuple<Payload...> payload;
};

}
