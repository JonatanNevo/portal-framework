//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <limits>
#include <type_traits>
#include <compare>

namespace portal
{
template <typename T>
concept IntegralEnum = std::is_integral_v<std::underlying_type_t<T>>;

template <IntegralEnum FlagBitsType>
struct FlagTraits;

/**
 * A Flags wrapper for transforming bit vectors into flags with bitwise operators.
 * Inspired by cpp vulkan's implementation
 * 
 * @tparam BitType the underlying bit enum type
 */
template <IntegralEnum BitType>
class Flags
{
public:
    using MaskType = std::underlying_type_t<BitType>;

    // constructors
    constexpr Flags(BitType bit) noexcept : mask(static_cast<MaskType>(bit)) {}
    constexpr explicit Flags(MaskType flags) noexcept : mask(flags) {}

    constexpr Flags() noexcept : mask(0) {}
    constexpr Flags(const Flags& other) noexcept = default;

    // relational operators
    auto operator<=>(const Flags&) const = default;

    // logical operator
    constexpr bool operator!() const noexcept
    {
        return !mask;
    }

    // bitwise operators
    constexpr Flags operator&(const Flags& other) const noexcept
    {
        return Flags(mask & other.mask);
    }

    constexpr Flags operator|(const Flags& other) const noexcept
    {
        return Flags(mask | other.mask);
    }

    constexpr Flags operator^(const Flags& other) const noexcept
    {
        return Flags(mask ^ other.mask);
    }

    constexpr Flags operator~() const noexcept
    {
        return Flags(mask ^ FlagTraits<BitType>::all_flags.mask);
    }

    // assignment operators
    constexpr Flags& operator=(const Flags& other) noexcept = default;

    constexpr Flags& operator|=(const Flags& other) noexcept
    {
        mask |= other.mask;
        return *this;
    }

    constexpr Flags& operator&=(const Flags& other) noexcept
    {
        mask &= other.mask;
        return *this;
    }

    constexpr Flags& operator^=(const Flags& other) noexcept
    {
        mask ^= other.mask;
        return *this;
    }

    // cast operators
    explicit constexpr operator bool() const noexcept
    {
        return !!mask;
    }

    explicit constexpr operator MaskType() const noexcept
    {
        return mask;
    }

    constexpr MaskType get() const noexcept
    {
        return mask;
    }

private:
    MaskType mask;
};

/**
 * Traits of the bit enum
 */
template <IntegralEnum FlagBitsType>
struct FlagTraits
{
    // if to support bitmask operations on the bit enum itself
    static constexpr bool is_bitmask = false;
    // A value that represents all the flags turned on, max int by default
    static constexpr Flags<FlagBitsType> all_flags = Flags<FlagBitsType>(std::numeric_limits<std::underlying_type_t<FlagBitsType>>::max());
};
}

// bitwise operators
template <portal::IntegralEnum BitType>
constexpr portal::Flags<BitType> operator&(BitType bit, const portal::Flags<BitType>& flags) noexcept
{
    return flags.operator&(bit);
}

template <portal::IntegralEnum BitType>
constexpr portal::Flags<BitType> operator|(BitType bit, const portal::Flags<BitType>& flags) noexcept
{
    return flags.operator|(bit);
}

template <portal::IntegralEnum BitType>
constexpr portal::Flags<BitType> operator^(BitType bit, const portal::Flags<BitType>& flags) noexcept
{
    return flags.operator^(bit);
}

// bitwise operators on BitType
template <portal::IntegralEnum BitType> requires portal::FlagTraits<BitType>::is_bitmask
constexpr portal::Flags<BitType> operator&(BitType lhs, BitType rhs) noexcept
{
    return portal::Flags<BitType>(lhs) & rhs;
}

template <portal::IntegralEnum BitType> requires portal::FlagTraits<BitType>::is_bitmask
constexpr portal::Flags<BitType> operator|(BitType lhs, BitType rhs) noexcept
{
    return portal::Flags<BitType>(lhs) | rhs;
}

template <portal::IntegralEnum BitType> requires portal::FlagTraits<BitType>::is_bitmask
constexpr portal::Flags<BitType> operator^(BitType lhs, BitType rhs) noexcept
{
    return portal::Flags<BitType>(lhs) ^ rhs;
}

template <portal::IntegralEnum BitType> requires portal::FlagTraits<BitType>::is_bitmask
constexpr portal::Flags<BitType> operator~(BitType bit) noexcept
{
    return ~(portal::Flags<BitType>(bit));
}
