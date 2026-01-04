//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <vulkan/vulkan.hpp>
#include <portal/core/log.h>

namespace portal::renderer::vulkan
{
/**
 * @class DebugMessenger
 * @brief Vulkan debug callback handler
 *
 * Routes validation layer messages to logging system and tracks error counts.
 */
class DebugMessenger
{
public:
    DebugMessenger();

    /** @brief Gets combined error and warning count */
    int get_error_and_warning_count() const;

    /** @brief Gets error count */
    int get_error_count() const;

    /** @brief Gets warning count */
    int get_warning_count() const;

    /** @brief Gets info message count */
    int get_info_count() const;

    /**
     * @brief Vulkan debug callback function
     * @param severity Message severity
     * @param message_type Message type
     * @param callback_data Validation layer data
     * @param data User data pointer
     * @return VK_FALSE to continue execution
     */
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT message_type,
        const vk::DebugUtilsMessengerCallbackDataEXT* callback_data,
        void* data
    );

protected:
    int error_count;
    int warning_count;
    int info_count;

    /** @brief Logs validation message and updates counters */
    vk::Bool32 log(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT message_type,
        const vk::DebugUtilsMessengerCallbackDataEXT* callback_data
    );

    /** @brief Converts Vulkan severity to spdlog level */
    spdlog::level::level_enum get_severity(const vk::DebugUtilsMessageSeverityFlagBitsEXT severity);
};
} // portal
