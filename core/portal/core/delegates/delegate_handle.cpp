//
// Created by thejo on 4/23/2025.
//

#include "delegate_handle.h"

#include <atomic>
#include <compare>

namespace portal
{
constexpr static uint32_t INVALID_ID = static_cast<unsigned int>(~0);
static std::atomic<uint32_t> next_id = 1;

static uint32_t get_next_id()
{
    const auto id = next_id.fetch_add(1, std::memory_order_relaxed);
    if (id == INVALID_ID)
    {
        next_id = 0;
    }
    return id;
}

constexpr DelegateHandle::DelegateHandle() noexcept:
    id(INVALID_ID)
{
}

DelegateHandle::DelegateHandle(bool generate_id) noexcept:
    id(get_next_id())
{
}

DelegateHandle::DelegateHandle(DelegateHandle&& other) noexcept:
    id(other.id)
{
    other.reset();
}

DelegateHandle& DelegateHandle::operator=(DelegateHandle&& other) noexcept
{
    id = other.id;
    other.reset();
    return *this;
}

DelegateHandle::operator bool() const noexcept
{
    return is_valid();
}

auto DelegateHandle::operator<=>(const DelegateHandle& other) const
{
    return id <=> other.id;
}

bool DelegateHandle::is_valid() const noexcept
{
    return id != INVALID_ID;
}

void DelegateHandle::reset() noexcept
{
    id = INVALID_ID;
}
} // portal
