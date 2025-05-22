//
// Created by thejo on 4/23/2025.
//

#pragma once
#include <cstdint>

namespace portal
{

class DelegateHandle
{
public:
    constexpr DelegateHandle() noexcept;
    explicit DelegateHandle(bool generate_id) noexcept;
    DelegateHandle(DelegateHandle&& other) noexcept;
    DelegateHandle& operator=(DelegateHandle&& other) noexcept;

    ~DelegateHandle() noexcept = default;
    DelegateHandle(const DelegateHandle& other) = default;
    DelegateHandle& operator=(const DelegateHandle& other) = default;

    explicit operator bool() const noexcept;
    auto operator<=>(const DelegateHandle& other) const;
    bool operator==(const DelegateHandle& other) const noexcept;

    bool is_valid() const noexcept;
    void reset() noexcept;

private:
    uint32_t id;
};

} // portal
