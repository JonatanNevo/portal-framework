//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <expected>
#include <filesystem>
#include "vulkan_init.h"

namespace portal::vulkan
{

vk::raii::ShaderModule load_shader_module(const std::filesystem::path& path, const vk::raii::Device& device);

}