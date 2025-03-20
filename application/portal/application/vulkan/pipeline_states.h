//
// Created by Jonatan Nevo on 04/03/2025.
//

#pragma once

#include "portal/application/vulkan/common.h"

namespace portal {
class Deserializer;
class Serializer;
}

namespace portal::vulkan
{
class PipelineLayout;
class RenderPass;

struct VertexInputState
{
    std::vector<vk::VertexInputBindingDescription> bindings;
    std::vector<vk::VertexInputAttributeDescription> attributes;

    void serialize(Serializer& ser) const;
    static VertexInputState deserialize(Deserializer& des);
};

struct InputAssemblyState
{
    vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
    vk::Bool32 primitive_restart_enable = VK_FALSE;

    void serialize(Serializer& ser) const;
    static InputAssemblyState deserialize(Deserializer& des);
};

struct RasterizationState
{
    vk::Bool32 depth_clamp_enable = VK_FALSE;
    vk::Bool32 rasterizer_discard_enable = VK_FALSE;
    vk::PolygonMode polygon_mode = vk::PolygonMode::eFill;
    vk::CullModeFlagBits cull_mode = vk::CullModeFlagBits::eBack;
    vk::FrontFace front_face = vk::FrontFace::eCounterClockwise;
    vk::Bool32 depth_bias_enable = VK_FALSE;

    void serialize(Serializer& ser) const;
    static RasterizationState deserialize(Deserializer& des);
};

struct ViewportState
{
    uint32_t viewport_count = 1;
    uint32_t scissor_count = 1;

    void serialize(Serializer& ser) const;
    static ViewportState deserialize(Deserializer& des);
};

struct MultisampleState
{
    vk::SampleCountFlagBits rasterization_samples = vk::SampleCountFlagBits::e1;
    vk::Bool32 sample_shading_enable = VK_FALSE;
    float min_sample_shading = 0.0f;
    vk::SampleMask sample_mask = 0;
    vk::Bool32 alpha_to_coverage_enable = VK_FALSE;
    vk::Bool32 alpha_to_one_enable = VK_FALSE;

    void serialize(Serializer& ser) const;
    static MultisampleState deserialize(Deserializer& des);
};

struct StencilOpState
{
    vk::StencilOp fail_op = vk::StencilOp::eReplace;
    vk::StencilOp pass_op = vk::StencilOp::eReplace;
    vk::StencilOp depth_fail_op = vk::StencilOp::eReplace;
    vk::CompareOp compare_op = vk::CompareOp::eNever;

    void serialize(Serializer& ser) const;
    static StencilOpState deserialize(Deserializer& des);
};

struct DepthStencilState
{
    vk::Bool32 depth_test_enable = VK_TRUE;
    vk::Bool32 depth_write_enable = VK_TRUE;
    // Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
    vk::CompareOp depth_compare_op = vk::CompareOp::eGreater;
    vk::Bool32 depth_bounds_test_enable = VK_FALSE;
    vk::Bool32 stencil_test_enable = VK_FALSE;
    StencilOpState front{};
    StencilOpState back{};

    void serialize(Serializer& ser) const;
    static DepthStencilState deserialize(Deserializer& des);
};

struct ColorBlendAttachmentState
{
    vk::Bool32 blend_enable = VK_FALSE;
    vk::BlendFactor src_color_blend_factor = vk::BlendFactor::eOne;
    vk::BlendFactor dst_color_blend_factor = vk::BlendFactor::eZero;
    vk::BlendOp color_blend_op = vk::BlendOp::eAdd;
    vk::BlendFactor src_alpha_blend_factor = vk::BlendFactor::eOne;
    vk::BlendFactor dst_alpha_blend_factor = vk::BlendFactor::eZero;
    vk::BlendOp alpha_blend_op = vk::BlendOp::eAdd;
    vk::ColorComponentFlags color_write_mask = vk::ColorComponentFlagBits::eR
        | vk::ColorComponentFlagBits::eG
        | vk::ColorComponentFlagBits::eB
        | vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendAttachmentState to_vk_attachment() const;

    void serialize(Serializer& ser) const;
    static ColorBlendAttachmentState deserialize(Deserializer& des);
};

struct ColorBlendState
{
    vk::Bool32 logic_op_enable = VK_FALSE;
    vk::LogicOp logic_op = vk::LogicOp::eClear;
    std::vector<ColorBlendAttachmentState> attachments;

    void serialize(Serializer& ser) const;
    static ColorBlendState deserialize(Deserializer& des);
};

/// Helper class to create specialization constants for a Vulkan pipeline. The state tracks a pipeline globally, and not per shader. Two shaders using the same constant_id will have the same data.
class SpecializationConstantState
{
public:
    void reset();
    [[nodiscard]] bool is_dirty() const;
    void clear_dirty();

    template <class T>
    void set_constant(uint32_t constant_id, const T& data);
    void set_constant(uint32_t constant_id, const std::vector<uint8_t>& value);
    void set_specialization_constant_state(const std::map<uint32_t, std::vector<uint8_t>>& state);

    [[nodiscard]] const std::map<uint32_t, std::vector<uint8_t>>& get_specialization_constant_state() const;

    void serialize(Serializer& serializer) const;
    static SpecializationConstantState deserialize(Deserializer& deserializer);

private:
    bool dirty = false;
    std::map<uint32_t, std::vector<uint8_t>> specialization_constant_state;
};

template <class T>
void SpecializationConstantState::set_constant(const std::uint32_t constant_id, const T& data)
{
    set_constant(constant_id, to_bytes(static_cast<std::uint32_t>(data)));
}

template <>
inline void SpecializationConstantState::set_constant<bool>(const std::uint32_t constant_id, const bool& data)
{
    set_constant(constant_id, to_bytes(static_cast<std::uint32_t>(data)));
}

class PipelineState
{
public:
    void reset();

    void set_pipeline_layout(PipelineLayout& new_pipeline_layout);
    void set_render_pass(const RenderPass& new_render_pass);
    void set_specialization_constant(uint32_t constant_id, const std::vector<uint8_t>& data);
    void set_vertex_input_state(const VertexInputState& new_vertex_input_state);
    void set_input_assembly_state(const InputAssemblyState& new_input_assembly_state);
    void set_rasterization_state(const RasterizationState& new_rasterization_state);
    void set_viewport_state(const ViewportState& new_viewport_state);
    void set_multisample_state(const MultisampleState& new_multisample_state);
    void set_depth_stencil_state(const DepthStencilState& new_depth_stencil_state);
    void set_color_blend_state(const ColorBlendState& new_color_blend_state);
    void set_subpass_index(uint32_t new_subpass_index);

    const PipelineLayout& get_pipeline_layout() const;
    const RenderPass* get_render_pass() const;
    const SpecializationConstantState& get_specialization_constant_state() const;
    const VertexInputState& get_vertex_input_state() const;
    const InputAssemblyState& get_input_assembly_state() const;
    const RasterizationState& get_rasterization_state() const;
    const ViewportState& get_viewport_state() const;
    const MultisampleState& get_multisample_state() const;
    const DepthStencilState& get_depth_stencil_state() const;
    const ColorBlendState& get_color_blend_state() const;
    uint32_t get_subpass_index() const;

    bool is_dirty() const;
    void clear_dirty();

    void serialize(Serializer& serializer) const;
    static PipelineState deserialize(Deserializer& deserializer);

private:
    bool dirty{false};
    PipelineLayout* pipeline_layout = nullptr;
    const RenderPass* render_pass = nullptr;
    SpecializationConstantState specialization_constant_state{};
    VertexInputState vertex_input_state{};
    InputAssemblyState input_assembly_state{};
    RasterizationState rasterization_state{};
    ViewportState viewport_state{};
    MultisampleState multisample_state{};
    DepthStencilState depth_stencil_state{};
    ColorBlendState color_blend_state{};
    uint32_t subpass_index{0U};
};
}
