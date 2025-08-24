//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/renderer/pipeline_builder.h"
#include "portal/engine/resources/resources/resource.h"
#include "../../shaders/shader.h"

namespace portal
{

namespace resources
{
    class GltfLoader;
}

class Pipeline final : public Resource
{
public:
    explicit Pipeline(const StringId& id): Resource(id) {}
    void copy_from(Ref<Resource> other) override;

    void set_shaders(WeakRef<Shader> vertex, WeakRef<Shader> fragment);
    void set_layout(const std::shared_ptr<vk::raii::PipelineLayout>& new_layout);
    void set_pipeline(const std::shared_ptr<vk::raii::Pipeline>& new_pipeline);

    [[nodiscard]] const vk::raii::Pipeline& get() const { return *pipeline; }
    [[nodiscard]] const vk::raii::PipelineLayout& get_layout() const { return *layout; }
    [[nodiscard]] vk::raii::PipelineLayout& get_layout() { return *layout; }
    [[nodiscard]] const vk::raii::Pipeline& get() { return *pipeline; }
    [[nodiscard]] Ref<Shader> get_shader(vk::ShaderStageFlagBits stage) const;

protected:
    std::shared_ptr<vk::raii::Pipeline> pipeline = nullptr;
    std::shared_ptr<vk::raii::PipelineLayout> layout = nullptr;

    WeakRef<Shader> vertex_shader;
    WeakRef<Shader> fragment_shader;
};
} // portal