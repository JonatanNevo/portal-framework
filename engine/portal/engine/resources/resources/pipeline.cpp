//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "pipeline.h"

namespace portal
{

void Pipeline::copy_from(const Ref<Resource> other)
{
    auto other_pipeline = other.as<Pipeline>();
    pipeline = other_pipeline->pipeline;
    layout = other_pipeline->layout;

    vertex_shader = other_pipeline->vertex_shader;
    fragment_shader = other_pipeline->fragment_shader;
}

void Pipeline::set_shaders(const WeakRef<Shader> vertex, const WeakRef<Shader> fragment)
{
    this->vertex_shader = vertex;
    this->fragment_shader = fragment;
}

void Pipeline::set_layout(const std::shared_ptr<vk::raii::PipelineLayout>& new_layout)
{
    layout = new_layout;
}

void Pipeline::set_pipeline(const std::shared_ptr<vk::raii::Pipeline>& new_pipeline)
{
    pipeline = new_pipeline;
}

void Pipeline::set_descriptor_set_layouts(std::vector<vk::raii::DescriptorSetLayout>&& new_layouts)
{
    descriptor_set_layouts = std::move(new_layouts);
}

Ref<Shader> Pipeline::get_shader(vk::ShaderStageFlagBits stage) const
{
    if (stage == vk::ShaderStageFlagBits::eVertex)
        return vertex_shader.lock();
    if (stage == vk::ShaderStageFlagBits::eFragment)
        return fragment_shader.lock();
    return nullptr;
}

std::pair<Ref<Shader>, Ref<Shader>> Pipeline::get_shaders() const {
    return std::make_pair(vertex_shader.lock(), fragment_shader.lock());
}


} // portal
