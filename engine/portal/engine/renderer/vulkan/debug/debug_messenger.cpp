//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "debug_messenger.h"

#include <unordered_set>

namespace portal::renderer::vulkan
{

const auto logger = Log::get_logger("Vulkan Debug");

constexpr const char* get_message_type(const vk::DebugUtilsMessageTypeFlagsEXT message_type)
{
    if (message_type & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)
        return "General";
    if (message_type & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
        return "Validation";
    if (message_type & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
        return "Performance";
    if (message_type & vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding)
        return "Devive Address Binding";

    return "Unknown";
}


// Workaround for IMGUI warnings (which does not use VMA)
const static std::unordered_set<uint32_t> skipped_messages{
    // BestPractices-vkAllocateMemory-small-allocation - ignored because im using VMA for memory management
    0xfd92477a,
    // BestPractices-vkBindImageMemory-small-dedicated-allocation - ignored because im using VMA for memory management
    0x44604b49,
    // BestPractices-vkBindBufferMemory-small-dedicated-allocation - ignored because im using VMA for memory management
    0x10b59d4b,
    // BestPractices-vkEndCommandBuffer-VtxIndexOutOfBounds - ignored because im using buffer pointer for vertex buffer
    0xc91ae640,
};

DebugMessenger::DebugMessenger() : error_count(0), warning_count(0), info_count(0) {}

int DebugMessenger::get_error_and_warning_count() const
{
    return error_count + warning_count;
}

int DebugMessenger::get_error_count() const
{
    return error_count;
}

int DebugMessenger::get_warning_count() const
{
    return warning_count;
}

int DebugMessenger::get_info_count() const
{
    return info_count;
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugMessenger::debug_callback(
    const vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    const vk::DebugUtilsMessageTypeFlagsEXT message_type,
    const vk::DebugUtilsMessengerCallbackDataEXT* callback_data,
    void* data
    )
{
    auto messenger = static_cast<DebugMessenger*>(data);
    return messenger->log(severity, message_type, callback_data);
}

vk::Bool32 renderer::vulkan::DebugMessenger::log(
    const vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    const vk::DebugUtilsMessageTypeFlagsEXT message_type,
    const vk::DebugUtilsMessengerCallbackDataEXT* callback_data
    )
{
    if (skipped_messages.contains(static_cast<uint32_t>(callback_data->messageIdNumber)))
        return vk::False;

    std::string labels;
    std::string objects;
    if (callback_data->cmdBufLabelCount)
    {
        labels = std::format("\tLabels({}): \n", callback_data->cmdBufLabelCount);
        for (uint32_t i = 0; i < callback_data->cmdBufLabelCount; ++i)
        {
            const auto& label = callback_data->pCmdBufLabels[i];
            const std::string color_str = std::format("[ {}, {}, {}, {} ]", label.color[0], label.color[1], label.color[2], label.color[3]);
            labels.append(
                std::format("\t\t- Command Buffer Label[{0}]: name: {1}, color: {2}\n", i, label.pLabelName ? label.pLabelName : "NULL", color_str)
                );
        }
    }

    if (callback_data->objectCount)
    {
        objects = std::format("\tObjects({}): \n", callback_data->objectCount);
        for (uint32_t i = 0; i < callback_data->objectCount; ++i)
        {
            const auto& object = callback_data->pObjects[i];
            objects.append(
                std::format(
                    "\t\t- Object[{0}] name: {1}, type: {2}, handle: {3:#x}\n",
                    i,
                    object.pObjectName ? object.pObjectName : "NULL",
                    vk::to_string(object.objectType),
                    object.objectHandle
                    )
                );
        }
    }

    const auto severity_enum = get_severity(severity);
    if (severity_enum <= spdlog::level::info)
    {
        logger->log(
            SOURCE_LOC,
            severity_enum,
            "{} - {}",
            get_message_type(message_type),
            callback_data->pMessage
            );
    }
    else
    {
        logger->log(
            SOURCE_LOC,
            severity_enum,
            "{} - {}\n{} {}",
            get_message_type(message_type),
            callback_data->pMessage,
            labels,
            objects
            );
    }

    return vk::False;
}

spdlog::level::level_enum DebugMessenger::get_severity(const vk::DebugUtilsMessageSeverityFlagBitsEXT severity)
{
    switch (severity)
    {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        info_count++;
        return spdlog::level::debug;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        info_count++;
        return spdlog::level::info;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        warning_count++;
        return spdlog::level::warn;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
        error_count++;
        return spdlog::level::err;
    }

    warning_count++;
    return spdlog::level::warn;
}
} // portal
