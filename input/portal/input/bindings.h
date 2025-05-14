//
// Created by thejo on 5/7/2025.
//

#pragma once

#include "portal/core/delegates/delegate.h"
#include "portal/input/input_types.h"

namespace portal::input
{

struct ActionBindingDelegate
{
    ActionBindingDelegate() :
        bound_delegate_type(BoundDelegate::Unbound)
    {
    };

    ActionBindingDelegate(Delegate<void, void> d) :
        delegate(std::make_shared<Delegate<void, void>>(std::move(d))), bound_delegate_type(BoundDelegate::Delegate)
    {
    };

    ActionBindingDelegate(Delegate<void, Key> d) :
        delegate_one_param(std::make_shared<Delegate<void, Key>>(std::move(d))), bound_delegate_type(BoundDelegate::DelegateWithKey)
    {
    };

    bool is_bound() const
    {
        switch (bound_delegate_type)
        {
        case BoundDelegate::Unbound:
            return false;
        case BoundDelegate::Delegate:
            return delegate->is_bound();
        case BoundDelegate::DelegateWithKey:
            return delegate_one_param->is_bound();
        }
        return false;
    }

    bool is_bound_to(const void* user_object) const
    {
        switch (bound_delegate_type)
        {
        case BoundDelegate::Unbound:
            return false;
        case BoundDelegate::Delegate:
            return delegate->is_bound_to(user_object);
        case BoundDelegate::DelegateWithKey:
            return delegate_one_param->is_bound_to(user_object);
        }
        return false;
    }

    const void* get_owner() const
    {
        switch (bound_delegate_type)
        {
        case BoundDelegate::Unbound:
            return nullptr;
        case BoundDelegate::Delegate:
            return delegate->get_owner();
        case BoundDelegate::DelegateWithKey:
            return delegate_one_param->get_owner();
        }
        return nullptr;
    }

    void execute(const Key& key)
    {
        switch (bound_delegate_type)
        {
        case BoundDelegate::Unbound:
            break;
        case BoundDelegate::Delegate:
            delegate->execute();
            break;
        case BoundDelegate::DelegateWithKey:
            delegate_one_param->execute(key);
            break;
        }
    }

private:
    /** Holds the delegate to call. */
    std::shared_ptr<portal::Delegate<void, void>> delegate;
    /** Holds the delegate that wants to know the key to call. */
    std::shared_ptr<portal::Delegate<void, Key>> delegate_one_param;

    enum class BoundDelegate : uint8_t
    {
        Unbound,
        Delegate,
        DelegateWithKey
    };

    BoundDelegate bound_delegate_type;
};

struct ActionBinding
{
    Key action_key;
    InputEvent event;
    ActionBindingDelegate delegate;

private:
    int32_t handle;
    bool is_paired;
};
}
