//
// Created by thejo on 4/23/2025.
//

#include "delegate.h"

namespace portal
{
constexpr DelegateBase::DelegateBase() noexcept
{
}

DelegateBase::~DelegateBase() noexcept
{
    release();
}

DelegateBase::DelegateBase(const DelegateBase& other)
{
    if (other.allocator.has_allocation())
    {
        allocator.allocate(other.allocator.get_size());
        other.get_delegate()->clone(allocator.get_allocation());
    }
}

DelegateBase::DelegateBase(DelegateBase&& other) noexcept:
    allocator(std::move(allocator))
{
}

DelegateBase& DelegateBase::operator=(const DelegateBase& other)
{
    release();
    if (other.allocator.has_allocation())
    {
        allocator.allocate(other.allocator.get_size());
        other.get_delegate()->clone(allocator.get_allocation());
    }
    return *this;
}

DelegateBase& DelegateBase::operator=(DelegateBase&& other) noexcept
{
    release();
    allocator = std::move(other.allocator);
    return *this;
}

const void* DelegateBase::get_owner() const
{
    if (allocator.has_allocation())
    {
        return get_delegate()->get_owner();
    }
    return nullptr;
}

size_t DelegateBase::get_size() const
{
    return allocator.get_size();
}

void DelegateBase::clear_if_bound_to(void* object)
{
    if (object && is_bound_to(object))
    {
        release();
    }
}

void DelegateBase::clear()
{
    release();
}

bool DelegateBase::is_bound() const
{
    return allocator.has_allocation();
}

bool DelegateBase::is_bound_to(const void* object) const
{
    if (object == nullptr || !allocator.has_allocation())
        return false;
    return get_delegate()->get_owner() == object;
}

void DelegateBase::release()
{
    if (allocator.has_allocation())
    {
        get_delegate()->~DelegateCallbackBase();
        allocator.release();
    }
}

DelegateCallbackBase* DelegateBase::get_delegate() const
{
    return static_cast<DelegateCallbackBase*>(allocator.get_allocation());
}
}
