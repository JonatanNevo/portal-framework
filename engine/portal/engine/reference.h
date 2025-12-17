//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <memory>

namespace portal
{
// I am aliasing shared_ptr as `Reference` for the option to easily extend this in the future with a custom class
template <typename T>
using Reference = std::shared_ptr<T>;

template <typename T>
using WeakReference = std::weak_ptr<T>;

template <typename T, typename... Args>
constexpr auto make_reference(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename To, typename From>
[[nodiscard]] Reference<To> reference_cast(const Reference<From>& ref)
{
    return std::dynamic_pointer_cast<To>(ref);
}
}
