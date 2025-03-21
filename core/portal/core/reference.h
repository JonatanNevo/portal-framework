//
// Created by Jonatan Nevo on 21/03/2025.
//

#pragma once

#include <atomic>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace portal
{
namespace reference
{
    void add_to_live_reference(void* instance);
    void remove_from_live_reference(void* instance);
    bool is_live(void* instance);
}

class CountedReference
{
public:
    virtual ~CountedReference() = default;

    void inc_ref_count() const;
    void dec_ref_count() const;

    [[nodiscard]] uint32_t get_ref_count() const;

private:
    mutable std::atomic<uint32_t> ref_count = 0;
};

template <typename T> requires std::is_base_of_v<CountedReference, T>
class Reference
{
public:
    Reference(): instance(nullptr) {}
    Reference(std::nullptr_t): instance(nullptr) {}

    Reference(T* instance): instance(instance)
    {
        inc_ref();
    }

    Reference(const Reference& other): instance(other.instance)
    {
        inc_ref();
    }

    Reference(Reference&& other) noexcept: instance(std::exchange(other.instance, nullptr)) {}

    template <typename TOther> requires std::is_base_of_v<CountedReference, TOther>
    explicit Reference(const Reference<TOther>& other)
    {
        instance = reinterpret_cast<T*>(other.instance);
        inc_ref();
    }

    template <typename TOther> requires std::is_base_of_v<CountedReference, TOther>
    explicit Reference(Reference<TOther>&& other) noexcept
    {
        instance = reinterpret_cast<T*>(std::exchange(other.instance, nullptr));
    }

    static Reference copy_without_increment(const Reference<T>& other)
    {
        Reference copy = nullptr;
        copy.instance = other.instance;
        return copy;
    }

    ~Reference()
    {
        dec_ref();
    }

    Reference<T>& operator=(std::nullptr_t n)
    {
        dec_ref();

        instance = nullptr;
        return *this;
    }

    Reference& operator=(const Reference& other)
    {
        if (this == &other)
            return *this;

        other.inc_ref();
        dec_ref();

        instance = other.instance;
        return *this;
    }

    template <typename TOther> requires std::is_base_of_v<CountedReference, TOther>
    Reference& operator=(const Reference<TOther>& other)
    {
        other.inc_ref();
        dec_ref();

        instance = other.instance;
        return *this;
    }

    template <typename TOther> requires std::is_base_of_v<CountedReference, TOther>
    Reference& operator=(Reference<TOther>&& other)
    {
        dec_ref();

        instance = other.instance;
        other.instance = nullptr;
        return *this;
    }

    explicit operator bool() { return instance != nullptr; }
    explicit operator bool() const { return instance != nullptr; }

    T* operator->() { return instance; }
    const T* operator->() const { return instance; }

    T& operator*() { return *instance; }
    const T& operator*() const { return *instance; }

    T* raw() { return instance; }
    const T* raw() const { return instance; }

    void reset(T* instance_override = nullptr)
    {
        dec_ref();
        instance = instance_override;
    }

    template <typename TOther> requires std::is_base_of_v<CountedReference, TOther>
    Reference<TOther> as() const
    {
        return Reference<TOther>(*this);
    }

    template <typename... Args>
    static Reference create(Args&&... args)
    {
#if defined(PORTAL_TRACE_MEMORY) && defined(PORTAL_PLATFORM_WINDOWS)
        return Reference<T>(new(typeid(T).name()) T(std::forward<Args>(args)...));
#else
        return Reference<T>(new T(std::forward<Args>(args)...));
#endif
    }

    bool operator==(const Reference& other) const { return instance == other.instance; }
    bool operator!=(const Reference& other) const { return instance != other.instance; }

    bool equal_object(const Reference& other) const
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
            instance->inc_ref_count();
            reference::add_to_live_reference(instance);
        }
    }
    void dec_ref() const
    {
        if (instance)
        {
            instance->dec_ref_count();

            if (instance->get_ref_count() == 0)
            {
                delete instance;
                reference::remove_from_live_reference(instance);
                instance = nullptr;
            }
        }
    }

private:
    mutable T* instance = nullptr;
};

template <typename T> requires std::is_base_of_v<CountedReference, T>
class WeakReference
{
public:
    WeakReference() = default;
    explicit WeakReference(Reference<T> reference): instance(reference.raw()) {}
    explicit WeakReference(T* instance): instance(instance) {}

    T* operator->() { return instance; }
    const T* operator->() const { return instance; }

    T& operator*() { return *instance; }
    const T& operator*() const { return *instance; }

    [[nodiscard]] bool is_valid() const { return instance ? is_live(instance) : false; }
    explicit operator bool() const { return is_valid(); }

    template <typename TOther> requires std::is_base_of_v<CountedReference, TOther>
    WeakReference<TOther> as() const
    {
        return WeakReference<TOther>(dynamic_cast<TOther*>(instance));
    }

private:
    T* instance = nullptr;
};
}
