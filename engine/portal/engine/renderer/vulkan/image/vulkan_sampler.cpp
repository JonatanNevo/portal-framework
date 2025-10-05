//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_sampler.h"

#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/engine/renderer/vulkan/vulkan_device.h"
#include "portal/engine/renderer/vulkan/vulkan_enum.h"

namespace portal::renderer::vulkan
{

VulkanSampler::VulkanSampler(const StringId& id, const SamplerSpecification& spec, Ref<VulkanDevice> device) : id(id), spec(spec)
{
    const vk::SamplerCreateInfo sampler_info{
        .magFilter = to_filter(spec.filter),
        .minFilter = to_filter(spec.filter),
        .mipmapMode = to_mipmap_mode(spec.mipmap_mode),
        .addressModeU = to_address_mode(spec.wrap),
        .addressModeV = to_address_mode(spec.wrap),
        .addressModeW = to_address_mode(spec.wrap),
        .anisotropyEnable = false,
        .maxAnisotropy = 1.f,
        .minLod = spec.min_lod,
        .maxLod = spec.max_lod,
        .borderColor = vk::BorderColor::eFloatOpaqueWhite,
    };

    sampler = device->create_sampler(sampler_info);
    device->set_debug_name(sampler, id);
}

vk::Sampler VulkanSampler::get_vk_sampler() const
{
    return sampler;
}

const SamplerSpecification& VulkanSampler::get_spec() const
{
    return spec;
}

} // portal
