//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <atomic>
#include <type_traits>

namespace portal
{

namespace ref_utils
{
    void add_to_live(void* instance);
    void remove_from_live(void* instance);
    bool is_live(void* instance);
    void clean_all_references();
}

class RefCounted
{
public:
    virtual ~RefCounted() = default;

    void inc_ref() const;
    void dec_ref() const;
    size_t get_ref() const;

private:
    mutable std::atomic<size_t> ref_count = 0;
};

/**
 * A Ref for a `RefCounted` object.
 * Calls the destructor for the underlying object only when the last Ref is destroyed.
 */
//TODO: support custom allocators
template <typename T>
class Ref
{

public:
    Ref(): instance(nullptr)
    {
        static_assert(std::is_base_of_v<RefCounted, T>, "T must inherit from RefCounted");
    }

    Ref(nullptr_t) : instance(nullptr) {}
    Ref(T* instance): instance(instance)
    {
        inc_ref();
    }

    template <typename T2> requires std::is_base_of_v<RefCounted, T2>
    Ref(const Ref<T2>& other)
    {
        instance = dynamic_cast<T*>(other.instance);
        inc_ref();
    }

    template <typename T2> requires std::is_base_of_v<RefCounted, T2>
    Ref(Ref<T2>&& other) noexcept
    {
        instance = dynamic_cast<T*>(other.instance);
        other.instance = nullptr;
    }

    static Ref copy_without_inc(const Ref& other)
    {
        Ref out{};
        out.instance = other.instance;
        return out;
    }

    ~Ref() noexcept
    {
        dec_ref();
    }

    Ref(const Ref& other): instance(other.instance)
    {
        inc_ref();
    }

    Ref& operator=(nullptr_t)
    {
        dec_ref();
        instance = nullptr;
        return *this;
    }

    Ref& operator=(const Ref& other)
    {
        if (this == &other)
            return *this;

        other.inc_ref();
        dec_ref();

        instance = other.instance;
        return *this;
    }

    template <typename T2> requires std::is_base_of_v<RefCounted, T2>
    Ref& operator=(const Ref<T2>& other)
    {
        other.inc_ref();
        dec_ref();

        instance = other.instance;
        return *this;
    }

    template <typename T2> requires std::is_base_of_v<RefCounted, T2>
    Ref& operator=(Ref<T2>&& other)
    {
        dec_ref();

        instance = dynamic_cast<T*>(other.instance);
        other.instance = nullptr;
        return *this;
    }

    operator bool()
    {
        return instance != nullptr;
    }

    operator bool() const
    {
        return instance != nullptr;
    }

    T* operator->()
    {
        return instance;
    }

    const T* operator->() const
    {
        return instance;
    }

    T& operator*()
    {
        return *instance;
    }

    const T& operator*() const
    {
        return *instance;
    }

    T* get()
    {
        return instance;
    }
    const T* get() const
    {
        return instance;
    }

    void reset(T* new_instance = nullptr)
    {
        dec_ref();
        instance = new_instance;
    }

    template <typename T2> requires std::is_base_of_v<RefCounted, T2>
    Ref<T2> as() const
    {
        return Ref<T2>(*this);
    }

    template <typename... Args>
    static Ref create(Args&&... args)
    {
        return Ref(new T(std::forward<Args>(args)...));
    }

    bool operator==(const Ref<T>& other) const
    {
        return instance == other.instance;
    }

    bool operator!=(const Ref<T>& other) const
    {
        return !(*this == other);
    }

    bool equals_object(const Ref<T>& other) const
    {
        if (!instance || !other.instance)
            return false;

        return *instance == *other.instance;
    }

private:
    void inc_ref() const
    {
        if (instance)
        {
            instance->inc_ref();
            ref_utils::add_to_live(instance);
        }
    }

    void dec_ref() const
    {
        if (instance)
        {
            instance->dec_ref();

            if (instance->get_ref() == 0)
            {
                delete instance;
                ref_utils::remove_from_live(instance);
                instance = nullptr;
            }
        }
    }

    template <typename T2>
    friend class Ref;
    template <typename T2>
    friend class WeakRef;
    mutable T* instance;
};

template <typename T>
class WeakRef
{
public:
    WeakRef() = default;

    WeakRef(Ref<T> ref)
    {
        static_assert(std::is_base_of_v<RefCounted, T>, "T must inherit from RefCounted");
        instance = ref.get();
    }

    WeakRef(T* instance) : instance(instance) {}

    WeakRef<T>& operator=(const Ref<T>& other)
    {
        instance = other.instance;
        return *this;
    }

    T* operator->() { return instance; }
    const T* operator->() const { return instance; }

    T& operator*() { return *instance; }
    const T& operator*() const { return *instance; }

    bool is_valid() const { return instance ? ref_utils::is_live(instance) : false; }
    operator bool() const { return is_valid(); }

    Ref<T> lock() const
    {
        return is_valid() ? Ref<T>(instance) : nullptr;
    }

    template <typename T2> requires std::is_base_of_v<RefCounted, T2>
    WeakRef<T2> as() const
    {
        return WeakRef<T2>(dynamic_cast<T2*>(instance));
    }

private:
    T* instance = nullptr;
};
} // portal
