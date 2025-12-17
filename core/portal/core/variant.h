//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <variant>

namespace portal
{
namespace details
{
    template <class... Ts>
    struct visitor : Ts...
    {
        using Ts::operator()...;
    };
}

template <typename Variant, class... Matchers>
decltype(auto) match(Variant&& variant, Matchers&&... matchers)
{
    return std::visit(details::visitor<Matchers>(matchers)..., std::forward<Variant>(variant));
}
}
