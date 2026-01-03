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

    template <class... Ts>
    visitor(Ts...) -> visitor<Ts...>;
}

template <typename Variant, class... Matchers>
void match(Variant&& variant, Matchers&&... matchers)
{
    std::visit(details::visitor{std::forward<Matchers>(matchers)...}, std::forward<Variant>(variant));
}
}
