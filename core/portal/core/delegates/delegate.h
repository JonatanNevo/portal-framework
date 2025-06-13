//
// Created by thejo on 4/23/2025.
//

#pragma once
#include <cstdlib>
#include <memory>
#include <tuple>

#include "delegate_callback.h"
#include "delegate_handle.h"
#include "inline_allocator.h"
#include "lambda_delegate.h"
#include "raw_delegate.h"
#include "shared_pointer_delegate.h"
#include "static_delegate.h"

#ifndef DELEGATE_INLINE_ALLOCATION_SIZE
#define DELEGATE_INLINE_ALLOCATION_SIZE 32
#endif

namespace portal
{
class DelegateBase
{
public:
    constexpr DelegateBase() noexcept = default;

    virtual ~DelegateBase() noexcept
    {
        release();
    }

    DelegateBase(const DelegateBase& other)
    {
        if (other.allocator.has_allocation())
        {
            allocator.allocate(other.allocator.get_size());
            other.get_delegate()->clone(allocator.get_allocation());
        }
    }

    DelegateBase(DelegateBase&& other) noexcept:
        allocator(std::move(other.allocator))
    {
    }

    DelegateBase& operator=(const DelegateBase& other)
    {
        release();
        if (other.allocator.has_allocation())
        {
            allocator.allocate(other.allocator.get_size());
            other.get_delegate()->clone(allocator.get_allocation());
        }
        return *this;
    }

    DelegateBase& operator=(DelegateBase&& other) noexcept
    {
        release();
        allocator = std::move(other.allocator);
        return *this;
    }

    /**
     * Returns the owner if exists
     */
    [[nodiscard]] const void* get_owner() const
    {
        if (allocator.has_allocation())
        {
            return get_delegate()->get_owner();
        }
        return nullptr;
    }

    /**
     * Returns the allocation size of the delegate
     */
    [[nodiscard]] size_t get_size() const
    {
        return allocator.get_size();
    }

    /**
     * Clears the delegate if it's bound to the given object
     */
    void clear_if_bound_to(void* object)
    {
        if (object && is_bound_to(object))
        {
            release();
        }
    }

    /**
     * Clears the cound delegate
     */
    void clear()
    {
        release();
    }

    /**
     * Checks if the delegate is bound to a function
     */
    [[nodiscard]] bool is_bound() const
    {
        return allocator.has_allocation();
    }

    /**
     * Checks if the delegate is bound to a given user object
     */
    bool is_bound_to(const void* object) const
    {
        if (object == nullptr || !allocator.has_allocation())
            return false;
        return get_delegate()->get_owner() == object;
    }

protected:
    /**
     * Releases the underlying allocation
     */
    void release()
    {
        if (allocator.has_allocation())
        {
            get_delegate()->~DelegateCallbackBase();
            allocator.release();
        }
    }

    /**
     * Gets the underlying delegate callback
     */
    [[nodiscard]] DelegateCallbackBase* get_delegate() const
    {
        return static_cast<DelegateCallbackBase*>(allocator.get_allocation());
    }

    InlineAllocator<DELEGATE_INLINE_ALLOCATION_SIZE> allocator;
};


/**
 * A single cast delegate the can be bound to a single function or member function.
 *
 * @tparam RetVal The return type of the underlying function
 * @tparam Args The argument types of the underlying function
 */
template <typename RetVal, typename... Args>
class Delegate final : public DelegateBase
{
private:
    template <typename T, typename... Payload>
    using ConstMemberFunction = typename delegates::MemberFunction<true, T, RetVal, Args..., Payload...>::Type;
    template <typename T, typename... Payload>
    using NonConstMemberFunction = typename delegates::MemberFunction<false, T, RetVal, Args..., Payload...>::Type;

public:
    using DelegateType = DelegateInterface<RetVal, Args...>;

    /* Delegate from instance and member function */
    template <typename T, typename... Payload>
    [[nodiscard]] static Delegate create_raw(T* object, NonConstMemberFunction<T, Payload...> function, Payload&&... args)
    {
        Delegate delegate;
        delegate.template bind<RawDelegate<false, T, RetVal(Args...), Payload...>>(object, function, std::forward<Payload>(args)...);
        return delegate;
    }

    template <typename T, typename... Payload>
    [[nodiscard]] static Delegate create_raw(T* object, ConstMemberFunction<T, Payload...> function, Payload&&... args)
    {
        Delegate delegate;
        delegate.template bind<RawDelegate<true, T, RetVal(Args...), Payload...>>(object, function, std::forward<Payload>(args)...);
        return delegate;
    }

    /* Delegate from static / global function, does not bind to an object */
    template <typename... Payload>
    [[nodiscard]] static Delegate create_static(RetVal (*function)(Args..., Payload...), Payload... args)
    {
        Delegate delegate;
        delegate.template bind<StaticDelegate<RetVal(Args...)>>(function, std::forward<Payload>(args)...);
        return delegate;
    }

    /* Delegate from a shared ptr */
    template <typename T, typename... Payload>
    [[nodiscard]] static Delegate create_shared_ptr(
        const std::shared_ptr<T>& object,
        NonConstMemberFunction<T, Payload...> function,
        Payload&&... args
    )
    {
        Delegate delegate;
        delegate.template bind<SharedPointerDelegate<false, T, RetVal(Args...), Payload...>>(object, function, std::forward<Payload>(args)...);
        return delegate;
    }

    template <typename T, typename... Payload>
    [[nodiscard]] static Delegate create_shared_ptr(const std::shared_ptr<T>& object, ConstMemberFunction<T, Payload...> function, Payload&&... args)
    {
        Delegate delegate;
        delegate.template bind<SharedPointerDelegate<true, T, RetVal(Args...), Payload...>>(object, function, std::forward<Payload>(args)...);
        return delegate;
    }

    /* Create a delegate from lambda */
    template <typename TLambda, typename... Payload>
    [[nodiscard]] static Delegate create_lambda(TLambda&& lambda, Payload... args)
    {
        Delegate delegate;
        using LambdaType = std::decay_t<TLambda>;
        delegate.template bind<LambdaDelegate<LambdaType, RetVal(Args...), Payload...>>(
            std::forward<LambdaType>(lambda),
            std::forward<Payload>(args)...
        );
        return delegate;
    }

    /* Binds to an instance and member function */
    template <typename T, typename... Payload>
    void bind_raw(T* object, NonConstMemberFunction<T, Payload...> function, Payload&&... args)
    {
        static_assert(!std::is_const_v<T>, "Cannot bind non const function to a const object");
        *this = create_raw(object, function, std::forward<Payload>(args)...);
    }

    template <typename T, typename... Payload>
    void bind_raw(T* object, ConstMemberFunction<T, Payload...> function, Payload&&... args)
    {
        *this = create_raw(object, function, std::forward<Payload>(args)...);
    }

    /* Binds to static or global function */
    template <typename... Payload>
    void bind_static(RetVal (*function)(Args..., Payload...), Payload&&... args)
    {
        *this = create_static(function, std::forward<Payload>(args)...);
    }

    /* Binds a lambda */
    template <typename TLambda, typename... Payload>
    void bind_lambda(TLambda&& function, Payload&&... args)
    {
        *this = create_lambda(std::forward<TLambda>(function), std::forward<Payload>(args)...);
    }

    /* Binds from a shared ptr */
    template <typename T, typename... Payload>
    void bind_shared_ptr(const std::shared_ptr<T>& object, NonConstMemberFunction<T, Payload...> function, Payload&&... args)
    {
        static_assert(!std::is_const_v<T>, "Cannot bind non const function to a const object");
        *this = create_shared_ptr(object, function, std::forward<Payload>(args)...);
    }

    template <typename T, typename... Payload>
    void bind_shared_ptr(const std::shared_ptr<T>& object, ConstMemberFunction<T, Payload...> function, Payload&&... args)
    {
        *this = create_shared_ptr(object, function, std::forward<Payload>(args)...);
    }

    /* Executes the delegate */
    RetVal execute(Args... args) const
    {
        PORTAL_ASSERT(allocator.has_allocation(), "Delegate is not bound to a function");
        return static_cast<DelegateType*>(allocator.get_allocation())->execute(std::forward<Args>(args)...);
    }

    // TODO: change return value to optional?
    RetVal execute_if_bound(Args... args) const
    {
        if (is_bound())
            return static_cast<DelegateType*>(allocator.get_allocation())->execute(std::forward<Args>(args)...);
        return RetVal();
    }

private:
    template <typename T, typename... Payload>
    void bind(Payload&&... args)
    {
        release();
        void* allocation = allocator.allocate(sizeof(T));
        new(allocation) T(std::forward<Payload>(args)...);
    }
};

/**
 * A multiple cast delegate that can be bound to multiple function or member function.
 * Note that currently multicast delegates cannot have return types
 *
 * @tparam Args The argument types of the underlying function
 */
template <typename... Args>
class MulticastDelegate final : public DelegateBase
{
public:
    using DelegateType = Delegate<void, Args...>;

private:
    /**
     * Holds a delegate and its handle to be used to map to multicast
     */
    struct DelegateHandlerPair
    {
        DelegateHandle handle;
        DelegateType callback;

        DelegateHandlerPair() :
            handle(false)
        {
        }

        DelegateHandlerPair(const DelegateHandle& handle, const DelegateType& callback) :
            handle(handle),
            callback(callback)
        {
        }

        DelegateHandlerPair(const DelegateHandle& handle, DelegateType&& callback) :
            handle(handle),
            callback(std::move(callback))
        {
        }
    };

    template <typename T, typename... Payload>
    using ConstMemberFunction = typename delegates::MemberFunction<true, T, void, Args..., Payload...>::Type;
    template <typename T, typename... Payload>
    using NonConstMemberFunction = typename delegates::MemberFunction<false, T, void, Args..., Payload...>::Type;

public:
    constexpr MulticastDelegate() noexcept = default;
    MulticastDelegate(const MulticastDelegate& other) = default;
    MulticastDelegate& operator=(const MulticastDelegate& other) = default;

    MulticastDelegate(MulticastDelegate&& other) noexcept :
        delegates(std::move(other.delegates))
    {
        locks.store(other.locks.load());
        other.locks.store(0);
    }

    MulticastDelegate& operator=(MulticastDelegate&& other) noexcept
    {
        delegates = std::move(other.delegates);
        locks.store(other.locks.load());
        other.locks.store(0);
        return *this;
    }

    /**
     * Add operator, adds a lambda to the multicast
     * @tparam T The lambda type
     * @param l the lambda instance
     * @return An handle to the delegate
     */
    template <typename T>
    DelegateHandle operator+=(T&& l)
    {
        return add(DelegateType::create_lambda(std::forward<T>(l)));
    }

    /**
     * Add operator, adds an already created delegate to the multicast
     */
    DelegateHandle operator+=(DelegateType&& delegate) noexcept
    {
        return add(std::forward<DelegateType>(delegate));
    }

    /**
     * Removes a delegate based on its handle
     *
     * @param handle The delegate handle
     * @return true of the delegate was found and removed
     */
    bool operator-=(DelegateHandle& handle)
    {
        return remove(handle);
    }

    /**
     * Adds a delegate to the multicast
     *
     * @param delegate the delegate to add
     * @return A handle to the delegate in the multicast
     */
    DelegateHandle add(DelegateType&& delegate) noexcept
    {
        // Favour an empty space over a possible array reallocation
        for (size_t i = 0; i < delegates.size(); ++i)
        {
            if (!delegates[i].handle.is_valid())
            {
                delegates[i] = DelegateHandlerPair(DelegateHandle(true), std::move(delegate));
                return delegates[i].handle;
            }
        }
        delegates.emplace_back(DelegateHandle(true), std::move(delegate));
        return delegates.back().handle;
    }

    /* Adds a member function */
    template <typename T, typename... Payload>
    DelegateHandle add_raw(T* object, NonConstMemberFunction<T, Payload...> function, Payload&&... args)
    {
        return add(DelegateType::create_raw(object, function, std::forward<Payload>(args)...));
    }

    template <typename T, typename... Payload>
    DelegateHandle add_raw(T* object, ConstMemberFunction<T, Payload...> function, Payload&&... args)
    {
        return add(DelegateType::create_raw(object, function, std::forward<Payload>(args)...));
    }

    /* Adds a static function */
    template <typename... Payload>
    DelegateHandle add_static(void (*function)(Args..., Payload...), Payload&&... args)
    {
        return add(DelegateType::create_static(function, std::forward<Payload>(args)...));
    }

    /* Adds a lambda */
    template <typename T, typename... Payload>
    DelegateHandle add_lambda(T&& function, Payload&&... args)
    {
        return add(DelegateType::create_lambda(std::forward<T>(function), std::forward<Payload>(args)...));
    }

    /* Adds a shared ptr */
    template <typename T, typename... Payload>
    DelegateHandle add_shared_ptr(const std::shared_ptr<T>& object, NonConstMemberFunction<T, Payload...> function, Payload&&... args)
    {
        return add(DelegateType::create_shared_ptr(object, function, std::forward<Payload>(args)...));
    }

    template <typename T, typename... Payload>
    DelegateHandle add_shared_ptr(const std::shared_ptr<T>& object, ConstMemberFunction<T, Payload...> function, Payload&&... args)
    {
        return add(DelegateType::create_shared_ptr(object, function, std::forward<Payload>(args)...));
    }

    /**
     * Remove all delegates bound to the given object
     *
     * @param object The object to remove references to
     */
    void remove_object(void* object)
    {
        if (!object)
            return;

        for (size_t i = 0; i < delegates.size(); ++i)
        {
            if (delegates[i].handle.is_valid() && delegates[i].callback.is_bound_to(object))
            {
                if (is_locked())
                    delegates[i].callback.clear();
                else
                {
                    std::swap(delegates[i], delegates.back());
                    delegates.pop_back();
                }
            }
        }
    }

    /**
     * Remove a delegate based on its handle
     *
     * @param handle The handle to remove
     * @return true if the handle was found and remove, false otherwise
     */
    bool remove(DelegateHandle& handle)
    {
        if (!handle.is_valid())
            return false;

        for (size_t i = 0; i < delegates.size(); ++i)
        {
            if (delegates[i].handle == handle)
            {
                if (is_locked())
                    delegates[i].callback.clear();
                else
                {
                    std::swap(delegates[i], delegates.back());
                    delegates.pop_back();
                }
                handle.reset();
                return true;
            }
        }
        return false;
    }

    /**
     * Checks if the delegate handle is bound to the multicast
     */
    [[nodiscard]] bool is_bound_to(const DelegateHandle& handle) const
    {
        if (!handle.is_valid())
            return false;

        for (size_t i = 0; i < delegates.size(); ++i)
        {
            if (delegates[i].handle == handle)
            {
                return true;
            }
        }

        return false;
    }

    /**
     * Removes all bound functions to the delegate
     */
    void remove_all()
    {
        if (is_locked())
        {
            for (DelegateHandlerPair& handler : delegates)
            {
                handler.callback.clear();
            }
        }
        else
        {
            delegates.clear();
        }
    }

    /**
     * Compresses the delegates vector to remove empty spaces
     */
    void compress(size_t max_space = 0)
    {
        // Cannot change the vector while the delegate is broadcasting
        if (is_locked())
            return;

        size_t to_delete = 0;
        for (size_t i = 0; i < delegates.size() - to_delete; ++i)
        {
            if (!delegates[i].handle.is_valid())
            {
                std::swap(delegates[i], delegates[delegates.size() - 1 - to_delete]);
                ++to_delete;
            }
        }

        if (to_delete > max_space)
        {
            delegates.resize(delegates.size() - to_delete);
        }
    }

    /**
     * Broadcasts the delegate to all bound functions
     */
    void broadcast(Args... args)
    {
        lock();
        for (auto& delegate_pair : delegates)
        {
            if (delegate_pair.handle.is_valid())
                delegate_pair.callback.execute(args...);
        }
        unlock();
    }

    size_t get_count() const
    {
        return delegates.size();
    }

private:
    void lock()
    {
        ++locks;
    }

    void unlock()
    {
        PORTAL_ASSERT(locks > 0, "Cannot unlock a delegate that is not locked");
        --locks;
    }

    /**
     * Checks if the delegate is locked (currently broadcasting), if it is, the order of the
     * delegates vector must not change
     *
     * @return True if the delegate is broadcasting
     */
    [[nodiscard]] bool is_locked() const
    {
        return locks > 0;
    }

private:
    std::vector<DelegateHandlerPair> delegates{};
    std::atomic<uint32_t> locks = 0;
};
} // portal

#define DECLARE_DELEGATE(name, ...) using name = portal::Delegate<void, __VA_ARGS__>;
#define DECLARE_DELEGATE_RET(name, ret, ...) using name = portal::Delegate<ret, __VA_ARGS__>;
