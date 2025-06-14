//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <format>
#include "portal/core/glm.h"

namespace std
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
} // namespace fmt
