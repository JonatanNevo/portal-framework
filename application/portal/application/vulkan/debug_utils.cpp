//
// Created by Jonatan Nevo on 02/03/2025.
//

#include "debug_utils.h"

#include "portal/application/vulkan/command_buffer.h"
#include "portal/application/vulkan/device.h"
#include "portal/core/assert.h"

namespace portal::vulkan
{
void DebugUtilsExtDebugUtils::set_debug_name(
	const vk::Device device,
	const vk::ObjectType object_type,
	const uint64_t object_handle,
	const char* name
) const
{
	const vk::DebugUtilsObjectNameInfoEXT name_info(object_type, object_handle, name);
	device.setDebugUtilsObjectNameEXT(name_info);
}

void DebugUtilsExtDebugUtils::set_debug_tag(
	const vk::Device device,
	const vk::ObjectType object_type,
	const uint64_t object_handle,
	const uint64_t tag_name,
	const void* tag_data,
	const size_t tag_data_size
) const
{
	const vk::DebugUtilsObjectTagInfoEXT tag_info(object_type, object_handle, tag_name, tag_data_size, tag_data);
	device.setDebugUtilsObjectTagEXT(tag_info);
}

void DebugUtilsExtDebugUtils::cmd_begin_label(const vk::CommandBuffer command_buffer, const char* name, glm::vec4 const color) const
{
	const vk::DebugUtilsLabelEXT label_info(name, reinterpret_cast<std::array<float, 4> const&>(*&color[0]));
	command_buffer.beginDebugUtilsLabelEXT(label_info);
}

void DebugUtilsExtDebugUtils::cmd_end_label(const vk::CommandBuffer command_buffer) const
{
	command_buffer.endDebugUtilsLabelEXT();
}

void DebugUtilsExtDebugUtils::cmd_insert_label(const vk::CommandBuffer command_buffer, const char* name, glm::vec4 const color) const
{
	const vk::DebugUtilsLabelEXT label_info(name, reinterpret_cast<std::array<float, 4> const&>(color[0]));
	command_buffer.insertDebugUtilsLabelEXT(label_info);
}

void DebugMarkerExtDebugUtils::set_debug_name(
	const vk::Device device,
	const vk::ObjectType object_type,
	const uint64_t object_handle,
	const char* name
) const
{
	const vk::DebugMarkerObjectNameInfoEXT name_info(vk::debugReportObjectType(object_type), object_handle, name);
	device.debugMarkerSetObjectNameEXT(name_info);
}

void DebugMarkerExtDebugUtils::set_debug_tag(
	const vk::Device device,
	const vk::ObjectType object_type,
	const uint64_t object_handle,
	const uint64_t tag_name,
	const void* tag_data,
	const size_t tag_data_size
) const
{
	const vk::DebugMarkerObjectTagInfoEXT tag_info(vk::debugReportObjectType(object_type), object_handle, tag_name, tag_data_size, tag_data);
	device.debugMarkerSetObjectTagEXT(tag_info);
}

void DebugMarkerExtDebugUtils::cmd_begin_label(const vk::CommandBuffer command_buffer, const char* name, glm::vec4 const color) const
{
	const vk::DebugMarkerMarkerInfoEXT marker_info(name, reinterpret_cast<std::array<float, 4> const&>(color[0]));
	command_buffer.debugMarkerBeginEXT(marker_info);
}

void DebugMarkerExtDebugUtils::cmd_end_label(const vk::CommandBuffer command_buffer) const
{
	command_buffer.debugMarkerEndEXT();
}

void DebugMarkerExtDebugUtils::cmd_insert_label(const vk::CommandBuffer command_buffer, const char* name, glm::vec4 const color) const
{
	const vk::DebugMarkerMarkerInfoEXT marker_info(name, reinterpret_cast<std::array<float, 4> const&>(color[0]));
	command_buffer.debugMarkerInsertEXT(marker_info);
}

ScopedDebugLabel::ScopedDebugLabel(
	const DebugUtils& debug_utils,
	vk::CommandBuffer command_buffer,
	const char* name,
	glm::vec4 color
) : debug_utils(&debug_utils), command_buffer(nullptr)
{
	if (name && *name != '\0')
	{
		PORTAL_CORE_ASSERT(command_buffer != nullptr, "Command buffer must be valid");
		this->command_buffer = command_buffer;

		debug_utils.cmd_begin_label(command_buffer, name, color);
	}
}

ScopedDebugLabel::ScopedDebugLabel(const CommandBuffer& command_buffer, const char* name, glm::vec4 color) : ScopedDebugLabel(
	command_buffer.get_device().get_debug_utils(),
	command_buffer.get_handle(),
	name,
	color
)
{}

ScopedDebugLabel::~ScopedDebugLabel()
{
	if (command_buffer != nullptr)
	{
		debug_utils->cmd_end_label(command_buffer);
	}
}
} // portal
