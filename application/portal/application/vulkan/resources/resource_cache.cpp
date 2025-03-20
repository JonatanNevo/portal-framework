//
// Created by Jonatan Nevo on 04/03/2025.
//

#include "resource_cache.h"
#include <set>

#include "portal/application/vulkan/descriptor_set.h"
#include "portal/application/vulkan/descriptor_set_layout.h"
#include "portal/application/vulkan/device.h"
#include "portal/application/vulkan/image_view.h"
#include "portal/application/vulkan/resources/hashing.h"
#include "portal/application/vulkan/shaders/shader_module.h"
#include "portal/serialization/serialize.h"

namespace portal::vulkan
{
namespace caching
{
    std::string ResourceIndex::to_string() const
    {
        return std::format("{}-{}", index, static_cast<uint8_t>(type));
    }

    ResourceIndex ResourceIndex::from_string(const std::string& str)
    {
        ResourceIndex result{};

        // Find the separator position
        const size_t separator_pos = str.find('-');
        if (separator_pos == std::string::npos)
        {
            throw std::runtime_error("Invalid ResourceIndex string format");
        }

        // Extract and parse the index part
        const std::string index_str = str.substr(0, separator_pos);
        result.index = std::stoull(index_str);

        // Extract and parse the type part
        const std::string type_str = str.substr(separator_pos + 1);
        result.type = static_cast<ResourceType>(std::stoul(type_str));

        return result;
    }


    void ShaderModuleCreateInfo::serialize(Serializer& serializer) const
    {
        serializer << stage << glsl_source.get_source() << entry_point << shader_variant.get_preamble() << shader_variant.get_processes();
    }

    portal::vulkan::caching::ShaderModuleCreateInfo ShaderModuleCreateInfo::deserialize(Deserializer& deserializer)
    {
        auto stage = deserializer.get_value<vk::ShaderStageFlagBits>();
        auto glsl_source = deserializer.get_value<ShaderSource>();
        auto entry_point = deserializer.get_value<std::string>();
        auto preamble = deserializer.get_value<std::string>();
        auto processes = deserializer.get_value<std::vector<std::string>>();

        ShaderModuleCreateInfo info{};
        info.stage = stage;
        info.glsl_source = glsl_source;
        info.entry_point = entry_point;
        info.shader_variant = ShaderVariant(std::move(preamble), std::move(processes));

        return info;
    }

    void PipelineLayoutCreateInfo::serialize(Serializer& serializer) const
    {
        serializer << shader_indices;
    }

    PipelineLayoutCreateInfo PipelineLayoutCreateInfo::deserialize(Deserializer& deserializer)
    {
        return {
            .shader_indices = deserializer.get_value<std::vector<size_t>>()
        };
    }

    void RenderPassCreateInfo::serialize(Serializer& serializer) const
    {
        serializer << attachments << load_store_infos << subpasses;
    }

    RenderPassCreateInfo RenderPassCreateInfo::deserialize(Deserializer& deserializer)
    {
        return {
            .attachments = deserializer.get_value<std::vector<Attachment>>(),
            .load_store_infos = deserializer.get_value<std::vector<LoadStoreInfo>>(),
            .subpasses = deserializer.get_value<std::vector<SubpassInfo>>()
        };
    }

    void PipelineCreateInfo::serialize(Serializer& serializer) const
    {
        serializer << pipeline_state<< pipeline_layout_index << render_pass_index;
    }

    PipelineCreateInfo PipelineCreateInfo::deserialize(Deserializer& deserializer)
    {
        return {
            .pipeline_state = deserializer.get_value<PipelineState>(),
            .pipeline_layout_index = deserializer.get_value<size_t>(),
            .render_pass_index = deserializer.get_value<size_t>()
        };
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
using namespace std::string_view_literals;


ResourceCache::ResourceCache(Device& device): device(device) {}

void ResourceCache::warmup(Deserializer& deserializer)
{
    const auto shader_module_infos = deserializer.get_value<std::vector<caching::ShaderModuleCreateInfo>>();
    auto pipeline_layout_infos = deserializer.get_value<std::vector<caching::PipelineLayoutCreateInfo>>();
    auto render_pass_infos = deserializer.get_value<std::vector<caching::RenderPassCreateInfo>>();
    auto pipeline_infos = deserializer.get_value<std::vector<caching::PipelineCreateInfo>>();

    std::vector<ShaderModule*> shader_modules;
    shader_modules.reserve(shader_module_infos.size());
    std::vector<PipelineLayout*> pipeline_layouts;
    pipeline_layouts.reserve(pipeline_layout_infos.size());
    std::vector<RenderPass*> render_passes;
    render_passes.reserve(render_pass_infos.size());


    for (const auto& shader_module_info : shader_module_infos)
    {
        auto& shader = request_shader_module(shader_module_info.stage, shader_module_info.glsl_source, shader_module_info.shader_variant);
        shader_modules.push_back(&shader);
    }

    for (const auto& [shader_indices] : pipeline_layout_infos)
    {
        std::vector<ShaderModule*> layout_shader_modules;
        for (const auto shader_index : shader_indices)
        {
            layout_shader_modules.push_back(shader_modules.at(shader_index));
        }
        auto& pipeline = request_pipeline_layout(layout_shader_modules);
        pipeline_layouts.push_back(&pipeline);
    }

    for (const auto& [attachments, load_store_infos, subpasses] : render_pass_infos)
    {
        auto& render_pass = request_render_pass(attachments, load_store_infos, subpasses);
        render_passes.push_back(&render_pass);
    }

    for (auto& pipeline_info : pipeline_infos)
    {
        const auto& pipeline_layout = pipeline_layouts.at(pipeline_info.pipeline_layout_index);
        const auto& render_pass = render_passes.at(pipeline_info.render_pass_index);

        auto& pipeline_state = pipeline_info.pipeline_state;
        pipeline_state.set_render_pass(*render_pass);
        pipeline_state.set_pipeline_layout(*pipeline_layout);
        request_graphics_pipeline(pipeline_state);
    }
}

void ResourceCache::serialize(Serializer& serializer) const
{
    serializer << shaders_create_infos << pipeline_layout_create_infos << render_pass_create_infos << pipeline_create_infos;
}

ShaderModule& ResourceCache::request_shader_module(
    const vk::ShaderStageFlagBits stage,
    const ShaderSource& glsl_source,
    const ShaderVariant& shader_variant
)
{
    constexpr auto entry_point = "main";
    std::lock_guard guard(shader_module_mutex);
    auto&& shader_module = request_resource(device, state.shader_modules, shader_module_to_index, stage, glsl_source, entry_point, shader_variant);
    shaders_create_infos.emplace_back(stage, glsl_source, std::string(entry_point), shader_variant);
    return shader_module;
}

PipelineLayout& ResourceCache::request_pipeline_layout(const std::vector<ShaderModule*>& shader_modules)
{
    std::lock_guard guard(pipeline_layout_mutex);
    auto&& pipeline_layout = request_resource(device, state.pipeline_layouts, pipeline_layout_to_index, shader_modules);

    std::vector<size_t> module_indices;
    for (auto& shader_module : shader_modules)
        module_indices.push_back(shader_module_to_index.at(shader_module));
    pipeline_layout_create_infos.emplace_back(module_indices);
    return pipeline_layout;
}

DescriptorSetLayout& ResourceCache::request_descriptor_set_layout(
    const uint32_t set_index,
    const std::vector<ShaderModule*>& shader_modules,
    const std::vector<ShaderResource>& set_resources
)
{
    std::lock_guard guard(descriptor_set_layout_mutex);
    return request_resource(device, state.descriptor_set_layouts, set_index, shader_modules, set_resources);
}

GraphicsPipeline& ResourceCache::request_graphics_pipeline(PipelineState& pipeline_state)
{
    std::lock_guard guard(graphics_pipeline_mutex);
    auto&& graphics_pipeline = request_resource(device, state.graphics_pipelines, pipeline_cache, pipeline_state);
    const auto pipeline_layout_index = pipeline_layout_to_index.at(&pipeline_state.get_pipeline_layout());
    const auto render_pass_index = render_pass_to_index.at(pipeline_state.get_render_pass());
    pipeline_create_infos.emplace_back(vk::PipelineCache{}, pipeline_state, pipeline_layout_index, render_pass_index);
    return graphics_pipeline;
}

ComputePipeline& ResourceCache::request_compute_pipeline(PipelineState& pipeline_state)
{
    std::lock_guard guard(compute_pipeline_mutex);
    return request_resource(device, state.compute_pipelines, pipeline_cache, pipeline_state);
}

DescriptorSet& ResourceCache::request_descriptor_set(
    DescriptorSetLayout& descriptor_set_layout,
    const BindingMap<vk::DescriptorBufferInfo>& buffer_infos,
    const BindingMap<vk::DescriptorImageInfo>& image_infos
)
{
    std::lock_guard guard(descriptor_set_mutex);
    auto& descriptor_pool = request_resource(device, state.descriptor_pools, descriptor_set_layout);
    return request_resource(device, state.descriptor_sets, descriptor_set_layout, descriptor_pool, buffer_infos, image_infos);
}

RenderPass& ResourceCache::request_render_pass(
    const std::vector<Attachment>& attachments,
    const std::vector<LoadStoreInfo>& load_store_infos,
    const std::vector<SubpassInfo>& subpasses
)
{
    std::lock_guard guard(render_pass_mutex);
    auto&& render_pass = request_resource(device, state.render_passes, render_pass_to_index, attachments, load_store_infos, subpasses);
    render_pass_create_infos.emplace_back(attachments, load_store_infos, subpasses);
    return render_pass;
}

Framebuffer& ResourceCache::request_framebuffer(const RenderTarget& render_target, const RenderPass& render_pass)
{
    std::lock_guard guard(framebuffer_mutex);
    return request_resource(device, state.framebuffers, render_target, render_pass);
}


void ResourceCache::set_pipeline_cache(vk::PipelineCache pipeline_cache)
{
    this->pipeline_cache = pipeline_cache;
}

void ResourceCache::update_descriptor_sets(const std::vector<ImageView>& old_views, const std::vector<ImageView>& new_views)
{
    // Find descriptor sets referring to the old image view
    std::vector<vk::WriteDescriptorSet> set_updates;
    std::set<size_t> matches;

    for (size_t i = 0; i < old_views.size(); ++i)
    {
        auto& old_view = old_views[i];
        auto& new_view = new_views[i];

        for (auto& [key, descriptor_set] : state.descriptor_sets)
        {
            auto& image_infos = descriptor_set.get_image_infos();
            for (auto& [binding, array] : image_infos)
            {
                for (auto& [array_element, image_info] : array)
                {
                    if (image_info.imageView == old_view.get_handle())
                    {
                        // Save key to remove old descriptor set
                        matches.insert(key);

                        // Update image info with new view
                        image_info.imageView = new_view.get_handle();
                        // Save struct for writing the update later
                        {
                            vk::WriteDescriptorSet write_descriptor_set{};
                            if (const auto binding_info = descriptor_set.get_layout().get_layout_binding(binding))
                            {
                                write_descriptor_set.dstBinding = binding;
                                write_descriptor_set.descriptorType = binding_info->descriptorType;
                                write_descriptor_set.pImageInfo = &image_info;
                                write_descriptor_set.dstSet = descriptor_set.get_handle();
                                write_descriptor_set.dstArrayElement = array_element;
                                write_descriptor_set.descriptorCount = 1;

                                set_updates.push_back(write_descriptor_set);
                            }
                            else
                            {
                                LOG_CORE_ERROR_TAG("Vulkan", "Shader layout set does not use image binding at #{}", binding);
                            }
                        }
                    }
                }
            }
        }
    }

    if (!set_updates.empty())
        device.get_handle().updateDescriptorSets(set_updates, {});

    // Delete old entries (moved out descriptor sets)
    for (auto& match : matches)
    {
        // Move out of the map
        auto it = state.descriptor_sets.find(match);
        auto descriptor_set = std::move(it->second);
        state.descriptor_sets.erase(match);

        // Generate new key
        size_t new_key = 0U;
        hash_param(new_key, descriptor_set.get_layout(), descriptor_set.get_buffer_infos(), descriptor_set.get_image_infos());

        // Add (key, resource) to the cache
        state.descriptor_sets.emplace(new_key, std::move(descriptor_set));
    }
}

void ResourceCache::clear_pipelines()
{
    state.graphics_pipelines.clear();
    state.compute_pipelines.clear();
}

void ResourceCache::clear_framebuffers()
{
    state.framebuffers.clear();
}

void ResourceCache::clear()
{
    state.shader_modules.clear();
    state.pipeline_layouts.clear();
    state.descriptor_sets.clear();
    state.descriptor_set_layouts.clear();
    state.render_passes.clear();
    clear_pipelines();
    clear_framebuffers();
}

const caching::ResourceCacheState& ResourceCache::get_internal_state() const
{
    return state;
}
} // portal
