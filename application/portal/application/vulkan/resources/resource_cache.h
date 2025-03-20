//
// Created by Jonatan Nevo on 04/03/2025.
//

#pragma once
#include <unordered_map>
#include <portal/serialization/serialize.h>

#include "portal/application/vulkan/common.h"
#include "portal/application/vulkan/descriptor_pool.h"
#include "portal/application/vulkan/descriptor_set.h"
#include "portal/application/vulkan/descriptor_set_layout.h"
#include "portal/application/vulkan/frame_buffer.h"
#include "portal/application/vulkan/pipeline.h"
#include "portal/application/vulkan/pipeline_states.h"
#include "portal/application/vulkan/shaders/shader_module.h"
#include "portal/application/vulkan/render_pass.h"
#include "portal/application/vulkan/pipeline_layout.h"
#include "portal/application/vulkan/render_target.h"

namespace portal::vulkan
{
class ImageView;
class Device;
struct SubpassInfo;
class RenderTarget;


namespace caching
{
    enum class ResourceType: uint8_t
    {
        ShaderModule,
        PipelineLayout,
        RenderPass,
        GraphicsPipeline
    };

    struct ResourceIndex
    {
        size_t index;
        ResourceType type;

        [[nodiscard]] std::string to_string() const;
        static ResourceIndex from_string(const std::string& str);
    };

    /**
     * @brief Struct to hold the internal state of the Resource Cache
     *
     */
    struct ResourceCacheState
    {
        std::unordered_map<size_t, ShaderModule> shader_modules;
        std::unordered_map<size_t, PipelineLayout> pipeline_layouts;
        std::unordered_map<size_t, DescriptorSetLayout> descriptor_set_layouts;
        std::unordered_map<size_t, DescriptorPool> descriptor_pools;
        std::unordered_map<size_t, RenderPass> render_passes;
        std::unordered_map<size_t, GraphicsPipeline> graphics_pipelines;
        std::unordered_map<size_t, ComputePipeline> compute_pipelines;
        std::unordered_map<size_t, DescriptorSet> descriptor_sets;
        std::unordered_map<size_t, Framebuffer> framebuffers;
    };

    struct ShaderModuleCreateInfo
    {
        vk::ShaderStageFlagBits stage;
        ShaderSource glsl_source;
        std::string entry_point;
        ShaderVariant shader_variant;

        void serialize(Serializer& serializer) const;
        static ShaderModuleCreateInfo deserialize(Deserializer& deserializer);
    };

    struct PipelineLayoutCreateInfo
    {
        std::vector<size_t> shader_indices;

        void serialize(Serializer& serializer) const;
        static PipelineLayoutCreateInfo deserialize(Deserializer& deserializer);
    };

    struct RenderPassCreateInfo
    {
        std::vector<Attachment> attachments;
        std::vector<LoadStoreInfo> load_store_infos;
        std::vector<SubpassInfo> subpasses;

        void serialize(Serializer& serializer) const;
        static RenderPassCreateInfo deserialize(Deserializer& deserializer);
    };

    struct PipelineCreateInfo
    {
        vk::PipelineCache pipeline_cache;
        PipelineState pipeline_state;
        size_t pipeline_layout_index;
        size_t render_pass_index;

        void serialize(Serializer& serializer) const;
        static PipelineCreateInfo deserialize(Deserializer& deserializer);
    };
}


/**
 * @brief Cache all sorts of Vulkan objects specific to a Vulkan device.
 * Supports serialization and deserialization of cached resources.
 * There is only one cache for all these objects, with several unordered_map of hash indices
 * and objects. For every object requested, there is a templated version on request_resource.
 * Some objects may need building if they are not found in the cache.
 *
 * The resource cache is also linked with ResourceRecord and ResourceReplay. Replay can warm-up
 * the cache on app startup by creating all necessary objects.
 * The cache holds pointers to objects and has a mapping from such pointers to hashes.
 * It can only be destroyed in bulk, single elements cannot be removed.
 */
class ResourceCache
{
public:
    ResourceCache(Device& device);

    ResourceCache(const ResourceCache&) = delete;
    ResourceCache(ResourceCache&&) = delete;
    ResourceCache& operator=(const ResourceCache&) = delete;
    ResourceCache& operator=(ResourceCache&&) = delete;

    void warmup(Deserializer& deserializer);
    void serialize(Serializer& serializer) const;

    ShaderModule& request_shader_module(vk::ShaderStageFlagBits stage, const ShaderSource& glsl_source, const ShaderVariant& shader_variant = {});
    PipelineLayout& request_pipeline_layout(const std::vector<ShaderModule*>& shader_modules);
    DescriptorSetLayout& request_descriptor_set_layout(
        const uint32_t set_index,
        const std::vector<ShaderModule*>& shader_modules,
        const std::vector<ShaderResource>& set_resources
    );
    GraphicsPipeline& request_graphics_pipeline(PipelineState& pipeline_state);
    ComputePipeline& request_compute_pipeline(PipelineState& pipeline_state);
    DescriptorSet& request_descriptor_set(
        DescriptorSetLayout& descriptor_set_layout,
        const BindingMap<vk::DescriptorBufferInfo>& buffer_infos,
        const BindingMap<vk::DescriptorImageInfo>& image_infos
    );
    RenderPass& request_render_pass(
        const std::vector<Attachment>& attachments,
        const std::vector<LoadStoreInfo>& load_store_infos,
        const std::vector<SubpassInfo>& subpasses
    );
    Framebuffer& request_framebuffer(
        const RenderTarget& render_target,
        const RenderPass& render_pass
    );

    void set_pipeline_cache(vk::PipelineCache pipeline_cache);
    /// @brief Update those descriptor sets referring to old views
    /// @param old_views Old image views referred by descriptor sets
    /// @param new_views New image views to be referred
    void update_descriptor_sets(const std::vector<ImageView>& old_views, const std::vector<ImageView>& new_views);

    void clear_pipelines();
    void clear_framebuffers();
    void clear();
    const caching::ResourceCacheState& get_internal_state() const;

private:
    Device& device;
    vk::PipelineCache pipeline_cache = VK_NULL_HANDLE;
    caching::ResourceCacheState state;
    std::mutex descriptor_set_mutex;
    std::mutex pipeline_layout_mutex;
    std::mutex shader_module_mutex;
    std::mutex descriptor_set_layout_mutex;
    std::mutex graphics_pipeline_mutex;
    std::mutex render_pass_mutex;
    std::mutex compute_pipeline_mutex;
    std::mutex framebuffer_mutex;

    std::unordered_map<const ShaderModule*, size_t> shader_module_to_index;
    std::unordered_map<const PipelineLayout*, size_t> pipeline_layout_to_index;
    std::unordered_map<const RenderPass*, size_t> render_pass_to_index;

    std::vector<caching::ShaderModuleCreateInfo> shaders_create_infos;
    std::vector<caching::PipelineLayoutCreateInfo> pipeline_layout_create_infos;
    std::vector<caching::RenderPassCreateInfo> render_pass_create_infos;
    std::vector<caching::PipelineCreateInfo> pipeline_create_infos;
};
} // portal
