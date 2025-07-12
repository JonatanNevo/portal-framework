//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <utility>

#include "delegate_callback.h"

namespace portal
{

template <typename TLambda, typename RetVal, typename... Args>
class LambdaDelegate;

template <typename TLambda, typename RetVal, typename... Args, typename... Payload>
class LambdaDelegate<TLambda, RetVal(Args...), Payload...> final : public DelegateInterface<RetVal, Args...>
{
public:
    explicit LambdaDelegate(TLambda&& lambda, Payload&&... payload) :
        lambda(std::forward<TLambda>(lambda)),
        payload(std::forward<Payload>(payload)...)
    {
    }

    explicit LambdaDelegate(const TLambda& lambda, const std::tuple<Payload...>& payload) :
        lambda(lambda),
        payload(payload)
    {
    }

    RetVal execute(Args&&... args) override
    {
        return execute_internal(std::forward<Args>(args)..., std::index_sequence_for<Payload...>());
    }

    void clone(void* destination) override
    {
        new(destination) LambdaDelegate(lambda, payload);
    }

private:
    template <std::size_t... Is>
    RetVal execute_internal(Args&&... args, std::index_sequence<Is...>)
    {
        return (RetVal)((lambda)(std::forward<Args>(args)..., std::get<Is>(payload)...));
    }

    TLambda lambda;
    std::tuple<Payload...> payload;
};
}
