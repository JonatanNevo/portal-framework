//
// Created by thejo on 4/23/2025.
//

#pragma once

namespace portal
{

namespace delegates
{
    template <bool IsConst, typename Object, typename RetVal, typename... Args>
    struct MemberFunction;

    template <typename Object, typename RetVal, typename... Args>
    struct MemberFunction<true, Object, RetVal, Args...>
    {
        using Type = RetVal(Object::*)(Args...) const;
    };

    template <typename Object, typename RetVal, typename... Args>
    struct MemberFunction<false, Object, RetVal, Args...>
    {
        using Type = RetVal(Object::*)(Args...);
    };
}

/**
 * A base class for all delegate callbacks.
 * This is not templated to allow for containers of delegates.
 */
class DelegateCallbackBase
{
public:
    DelegateCallbackBase() = default;
    virtual ~DelegateCallbackBase() noexcept = default;
    virtual const void* get_owner() const { return nullptr; }
    virtual void clone(void* destination) = 0;
};


/** Signature for a delegate */
template <typename RetVal, typename... Args>
class DelegateInterface : public DelegateCallbackBase
{
public:
    virtual RetVal execute(Args&&... args) = 0;
};


}
