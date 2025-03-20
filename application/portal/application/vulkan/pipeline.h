//
// Created by Jonatan Nevo on 09/03/2025.
//

#pragma once
#include "portal/application/vulkan/pipeline_states.h"
#include "portal/application/vulkan/base/vulkan_resource.h"

namespace portal::vulkan
{
class Device;

class Pipeline : public VulkanResource<vk::Pipeline>
{
public:
    Pipeline(Device& device);
    Pipeline(Pipeline&& other) noexcept;
    ~Pipeline() override;

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline& operator=(Pipeline&&) = delete;

    const PipelineState& get_state() const;

protected:
    PipelineState state;
};


class ComputePipeline final : public Pipeline
{
public:
    ComputePipeline(
        Device& device,
        vk::PipelineCache pipeline_cache,
        PipelineState& pipeline_state
    );
};

class GraphicsPipeline final : public Pipeline
{
public:
    GraphicsPipeline(
        Device& device,
        vk::PipelineCache pipeline_cache,
        PipelineState& pipeline_state
    );
};
} // portal
