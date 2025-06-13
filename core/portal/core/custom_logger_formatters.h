//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/glm.h"
#include "portal/core/common.h"

namespace fmt
{
template <>
struct formatter<glm::vec2>
{
    char presentation = 'f';

    constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin())
    {
        auto it = ctx.begin();
        const auto end = ctx.end();
        if (it != end && (*it == 'f' || *it == 'e'))
            presentation = *it++;

        if (it != end && *it != '}')
            throw format_error("invalid format");

        return it;
    }

    template <typename FormatContext>
    auto format(const glm::vec2& vec, FormatContext& ctx) const -> decltype(ctx.out())
    {
        return presentation == 'f'
                   ? fmt::format_to(ctx.out(), "({:.3f}, {:.3f})", vec.x, vec.y)
                   : fmt::format_to(ctx.out(), "({:.3e}, {:.3e})", vec.x, vec.y);
    }
};

template <>
struct formatter<glm::vec3>
{
    char presentation = 'f';

    constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin())
    {
        auto it = ctx.begin();
        const auto end = ctx.end();
        if (it != end && (*it == 'f' || *it == 'e'))
            presentation = *it++;

        if (it != end && *it != '}')
            throw format_error("invalid format");

        return it;
    }

    template <typename FormatContext>
    auto format(const glm::vec3& vec, FormatContext& ctx) const -> decltype(ctx.out())
    {
        return presentation == 'f'
                   ? fmt::format_to(ctx.out(), "({:.3f}, {:.3f}, {:.3f})", vec.x, vec.y, vec.z)
                   : fmt::format_to(ctx.out(), "({:.3e}, {:.3e}, {:.3e})", vec.x, vec.y, vec.z);
    }
};

template <>
struct formatter<glm::vec4>
{
    char presentation = 'f';

    constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin())
    {
        auto it = ctx.begin();
        const auto end = ctx.end();
        if (it != end && (*it == 'f' || *it == 'e'))
            presentation = *it++;

        if (it != end && *it != '}')
            throw format_error("invalid format");

        return it;
    }

    template <typename FormatContext>
    auto format(const glm::vec4& vec, FormatContext& ctx) const -> decltype(ctx.out())
    {
        return presentation == 'f'
                   ? fmt::format_to(ctx.out(), "({:.3f}, {:.3f}, {:.3f}, {:.3f})", vec.x, vec.y, vec.z, vec.w)
                   : fmt::format_to(ctx.out(), "({:.3e}, {:.3e}, {:.3e}, {:.3e})", vec.x, vec.y, vec.z, vec.w);
    }
};

template <>
struct formatter<portal::uint128_t>
{
    char presentation = 'd';

    constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin())
    {
        auto it = ctx.begin();
        const auto end = ctx.end();
        if (it != end && (*it == 'd' || *it == 'x' || *it == 'b'))
            presentation = *it++;

        if (it != end && *it != '}')
            throw format_error("invalid format");

        return it;
    }

    template <typename FormatContext>
    auto format(const portal::uint128_t& value, FormatContext& ctx) const -> decltype(ctx.out())
    {
        if (presentation == 'x')
        {
            if (value.hi == 0)
                return fmt::format_to(ctx.out(), "{:x}", value.lo);
            return fmt::format_to(ctx.out(), "{:x}{:016x}", value.hi, value.lo);
        }
        else if (presentation == 'b')
        {
            // Binary representation
            if (value.hi == 0)
            {
                return fmt::format_to(ctx.out(), "0b{:b}", value.lo);
            }
            else
            {
                std::string bin_hi = fmt::format("{:b}", value.hi);
                std::string bin_lo = fmt::format("{:064b}", value.lo);
                return fmt::format_to(ctx.out(), "0b{}{}", bin_hi, bin_lo);
            }
        }
        else
        {
            // Decimal format (default)
            if (value.hi == 0)
            {
                return fmt::format_to(ctx.out(), "{}", value.lo);
            }
            else
            {
                std::string result;
                portal::uint128_t temp = value;
                while (temp != 0)
                {
                    portal::uint128_t remainder = temp % 10;
                    result.insert(0, 1, '0' + static_cast<char>(remainder.lo));
                    temp = temp / 10;
                }
                return result.empty() ? fmt::format_to(ctx.out(), "0") : fmt::format_to(ctx.out(), "{}", result);
            }
        }
    }
};
} // namespace fmt
