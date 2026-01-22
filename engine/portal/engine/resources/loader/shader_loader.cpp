//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "shader_loader.h"

#include <numeric>

#include <portal/core/strings/string_utils.h>

#include "portal/engine/renderer/renderer_context.h"
#include "portal/engine/resources/resource_registry.h"
#include "portal/engine/resources/source/resource_source.h"
#include "portal/engine/renderer/shaders/shader.h"
#include "portal/engine/renderer/vulkan/vulkan_shader.h"

namespace portal::resources
{
static auto logger = Log::get_logger("Resources");

ShaderLoader::ShaderLoader(ResourceRegistry& registry, const renderer::vulkan::VulkanContext& context) : ResourceLoader(registry), context(context)
{}


Reference<Resource> ShaderLoader::load(const SourceMetadata& meta, const ResourceSource& source)
{
    if (meta.format == SourceFormat::Shader)
        return load_shader(meta, source);
    if (meta.format == SourceFormat::PrecompiledShader)
        return load_precompiled_shader(meta, source);

    LOGGER_ERROR("Unknown shader format: {}", meta.format);
    return nullptr;
}

Reference<Resource> ShaderLoader::load_shader(const SourceMetadata& meta, const ResourceSource& source) const
{
    auto shader = make_reference<renderer::vulkan::VulkanShader>(meta.resource_id, context);
    // TODO: use global shader path somehow
    shader->load_source(source.load(), meta.source.string);
    return shader;
}

Reference<Resource> ShaderLoader::load_precompiled_shader(const SourceMetadata&, const ResourceSource&) const
{
    // TODO: load shader cache from disk / memory
    // const auto builder = vulkan::DescriptorLayoutBuilder{}
    //                      .add_binding(0, vk::DescriptorType::eUniformBuffer)
    //                      .add_binding(1, vk::DescriptorType::eCombinedImageSampler)
    //                      .add_binding(2, vk::DescriptorType::eCombinedImageSampler);
    // shader->descriptor_layout = std::make_shared<vk::raii::DescriptorSetLayout>(
    //     context->create_descriptor_set_layout(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, builder)
    //     );
    // shader->code = Buffer::copy(data);
    //
    // shader->shader_data[vk::ShaderStageFlagBits::eVertex] = {
    //     .push_constant_range = vk::PushConstantRange{
    //         .stageFlags = vk::ShaderStageFlagBits::eVertex,
    //         .offset = 0,
    //         .size = sizeof(vulkan::GPUDrawPushConstants)
    //     },
    //     .shader_module = std::make_shared<vk::raii::ShaderModule>(context->create_shader_module(shader->code)),
    //     .entry_point.h = "vert_main"
    // };
    //
    // shader->shader_data[vk::ShaderStageFlagBits::eFragment] = {
    //     .push_constant_range = {},
    //     .shader_module = std::make_shared<vk::raii::ShaderModule>(context->create_shader_module(shader->code)),
    //     .entry_point.h = "frag_main"
    // };

    return nullptr;
}
} // portal
