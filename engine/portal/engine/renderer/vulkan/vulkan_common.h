//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

namespace portal
{
template <typename T>
concept VkRaiiObject = requires { typename std::remove_reference_t<T>::CType; typename std::remove_reference_t<T>::CppType; };

template <typename T>
concept VkCppHandle = requires { typename std::remove_reference_t<T>::CType; } && (!VkRaiiObject<T>);

// vk::raii:: types (e.g., vk::raii::Instance)
template <VkRaiiObject T>
uint64_t to_uint64(T& obj)
{
    using CType = typename std::remove_reference_t<T>::CType;
    return reinterpret_cast<uint64_t>(static_cast<CType>(*obj));
}

// vk:: handle wrappers (e.g., vk::Instance, vk::Buffer)
template <VkCppHandle T>
uint64_t to_uint64(T obj)
{
    using CType = typename std::remove_reference_t<T>::CType;
    return reinterpret_cast<uint64_t>(static_cast<CType>(obj));
}

// Raw C handles (VkInstance, VkBuffer, VmaAllocation, etc.) and generic fallback.
// No introspection of incomplete types – just cast pointer or integral.
template <typename T> requires (!VkRaiiObject<T> && !VkCppHandle<T>)
uint64_t to_uint64(T obj)
{
    if constexpr (std::is_pointer_v<std::remove_cv_t<T>>)
        return reinterpret_cast<uint64_t>(obj);
    else if constexpr (std::is_integral_v<std::remove_cv_t<T>>)
        return static_cast<uint64_t>(obj);
    else
        return reinterpret_cast<uint64_t>(obj);
}
}

#define VK_HANDLE_CAST(raii_obj) portal::to_uint64(raii_obj)
