//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/delegates/delegate.h"
#include "portal/input/input_types.h"

namespace portal {
class Input;
}

namespace portal::input
{

struct ActionBindingDelegate
{
    ActionBindingDelegate() :
        bound_delegate_type(BoundDelegate::Unbound)
    {
    };

    explicit ActionBindingDelegate(Delegate<void> d) :
        delegate(std::make_shared<Delegate<void>>(std::move(d))), bound_delegate_type(BoundDelegate::Delegate)
    {
    };

    explicit ActionBindingDelegate(Delegate<void, Key> d) :
        delegate_one_param(std::make_shared<Delegate<void, Key>>(std::move(d))), bound_delegate_type(BoundDelegate::DelegateWithKey)
    {
    };

    [[nodiscard]] bool is_bound() const
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

    [[nodiscard]] const void* get_owner() const
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

    template< class T >
    void bind_delegate(T* object, typename Delegate<void>::NonConstMemberFunction<T> func)
    {
        unbind();
        bound_delegate_type = BoundDelegate::Delegate;
        delegate = std::make_shared<Delegate<void>>(Delegate<void>::create_raw(object, func));
    }

    template< class T >
    void bind_delegate(T* object, typename Delegate<void, Key>::NonConstMemberFunction<T> func)
    {
        unbind();
        bound_delegate_type = BoundDelegate::DelegateWithKey;
        delegate_one_param = std::make_shared<Delegate<void, Key>>(Delegate<void, Key>::create_raw(object, func));
    }

    void unbind()
    {
        switch(bound_delegate_type)
        {
        case BoundDelegate::Delegate:
            delegate->clear();
            break;

        case BoundDelegate::DelegateWithKey:
            delegate_one_param->clear();
            break;
        default:
            break;
        }
        bound_delegate_type = BoundDelegate::Unbound;
    }


private:
    /** Holds the delegate to call. */
    std::shared_ptr<portal::Delegate<void>> delegate;
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

    ActionBinding(): action_key(Keys::Invalid), event(InputEvent::Pressed), handle(-1), paired(false) {}
    ActionBinding(Key key, InputEvent event): action_key(key), event(event), handle(-1), paired(false) {}

    Key get_key() const { return action_key; }
    int32_t get_handle() const { return handle; }
    bool is_paired() const { return paired; }

    bool operator==(const ActionBinding& rhs) const
    {
        return is_valid() && handle == rhs.handle;
    }

    // TODO: remove magic
    bool is_valid() const { return handle != -1; }

    void generate_new_handle();

private:
    int32_t handle;
    bool paired;

    friend class portal::Input;
};

}
