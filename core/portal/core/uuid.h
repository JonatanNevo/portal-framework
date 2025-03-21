//
// Created by Jonatan Nevo on 21/03/2025.
//

#pragma once
#include <cstdint>
#include <cstddef>

#include <functional>
#include <memory>

namespace portal
{
class UUID
{
public:
    UUID();
    UUID(uint64_t uuid);
    UUID(const UUID& other);

    operator uint64_t() { return uuid; }
    operator const uint64_t() const { return uuid; }

private:
    uint64_t uuid;
};

class UUID32
{
public:
    UUID32();
    UUID32(uint32_t uuid);
    UUID32(const UUID32& other);

    operator uint32_t() { return uuid; }
    operator const uint32_t() const { return uuid; }

private:
    uint32_t uuid;
};
} // portal

namespace std
{
template <>
struct hash<portal::UUID>
{
    std::size_t operator()(const portal::UUID& uuid) const noexcept
    {
        return hash<uint64_t>{}(uuid);;
    }
};

template <>
struct hash<portal::UUID32>
{
    std::size_t operator()(const portal::UUID32& uuid) const noexcept
    {
        return hash<uint32_t>()(uuid);
    }
};
}
