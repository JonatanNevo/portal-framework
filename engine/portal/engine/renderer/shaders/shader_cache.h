//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <filesystem>

#include "portal/engine/resources/resources/resource.h"
#include "portal/engine/renderer/shaders/shader.h"
#include "portal/engine/renderer/shaders/shader_compiler.h"

namespace portal::renderer
{

/**
 * Generates and caches `Shader` resource for multiple materials per permutation.
 */
class ShaderCache final : public Resource
{
public:
    explicit ShaderCache(const StringId& id);

    void load_source(Buffer&& new_source);
    /**
     * Compiles the shader with a given list of permutations (defines)
     *
     * @param permutations A list of permutations (name, value)
     * @return The hash to fetch the shader code with.
     */
    uint64_t compile_with_permutations(std::vector<ShaderDefine>& permutations);

    WeakRef<Shader> get_shader(uint64_t shader_hash, const std::shared_ptr<renderer::vulkan::GpuContext>& context);

private:
    uint64_t calculate_permutations_hash(std::vector<ShaderDefine>& permutations);

private:
    std::filesystem::path source_path;
    Buffer source;
    std::unordered_map<uint64_t, CompiledShader> shaders;
    std::unordered_map<uint64_t, Ref<Shader>> shader_map;
};

} // portal
