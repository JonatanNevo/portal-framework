//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

namespace portal
{
/**
 * @concept VkRaiiObject
 * @brief Vulkan RAII object type
 *
 * Matches vk::raii:: types (e.g., vk::raii::Instance).
 */
template <typename T>
concept VkRaiiObject = requires { typename std::remove_reference_t<T>::CType; typename std::remove_reference_t<T>::CppType; };

/**
 * @concept VkCppHandle
 * @brief Vulkan C++ handle wrapper
 *
 * Matches vk:: types (e.g., vk::Instance, vk::Buffer).
 */
template <typename T>
concept VkCppHandle = requires { typename std::remove_reference_t<T>::CType; } && (!VkRaiiObject<T>);

/**
 * @brief Converts vk::raii object to uint64
 * @param obj RAII Vulkan object
 * @return Handle as uint64
 */
template <VkRaiiObject T>
uint64_t to_uint64(T& obj)
{
    using CType = typename std::remove_reference_t<T>::CType;
    return reinterpret_cast<uint64_t>(static_cast<CType>(*obj));
}

/**
 * @brief Converts vk:: handle to uint64
 * @param obj Vulkan C++ handle
 * @return Handle as uint64
 */
template <VkCppHandle T>
uint64_t to_uint64(T obj)
{
    using CType = typename std::remove_reference_t<T>::CType;
    return reinterpret_cast<uint64_t>(static_cast<CType>(obj));
}

/**
 * @brief Converts raw C handle to uint64
 * @param obj Vulkan C handle or pointer
 * @return Handle as uint64
 */
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

/** @brief Converts Vulkan handle to uint64 for debugging */
#define VK_HANDLE_CAST(raii_obj) portal::to_uint64(raii_obj)
