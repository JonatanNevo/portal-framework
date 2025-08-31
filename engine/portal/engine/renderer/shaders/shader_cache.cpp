//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "shader_cache.h"

#include "portal/engine/renderer/shaders/shader_compiler.h"
#include "portal/engine/strings/hash.h"

namespace portal::renderer
{

static auto logger = Log::get_logger("Shader");

ShaderCache::ShaderCache(const StringId& id) : Resource(id)
{}

void ShaderCache::load_source(Buffer&& new_source)
{
    source = new_source;
}

uint64_t ShaderCache::compile_with_permutations(std::vector<ShaderDefine>& permutations)
{
    const auto permutations_hash = calculate_permutations_hash(permutations);

    if (!shaders.contains(permutations_hash))
    {
        LOGGER_DEBUG("Compiling shader variant: {} [{}]", id, permutations_hash);
        ShaderCompiler compiler;
        shaders[permutations_hash] = compiler.compile({.name = id, .shader_path = source_path, .shader_data = source, .defines = permutations});
    }

    return permutations_hash;
}

WeakRef<Shader> ShaderCache::get_shader(const uint64_t shader_hash, const std::shared_ptr<renderer::vulkan::GpuContext>& context)
{
    if (!shaders.contains(shader_hash))
    {
        LOGGER_ERROR("Shader variant not found: {}", id);
        return {};
    }

    if (!shader_map.contains(shader_hash))
    {
        // Create shader variant
        auto [shader_iter, success] = shader_map.emplace(shader_hash, Ref<Shader>::create(id));
        if (!success)
        {
            LOGGER_ERROR("Failed to create shader variant: {}", id);
            return {};
        }
        auto shader = shader_iter->second;

        auto shader_data = shaders[shader_hash];
        shader->set_shader_source(&shader_data, context.get());
    }

    return shader_map[shader_hash];
}


uint64_t ShaderCache::calculate_permutations_hash(std::vector<ShaderDefine>& permutations)
{
    uint64_t hash = id.id;
    for (auto& permutation : permutations.begin())
    {
        hash ^= hash::rapidhash(permutation.name);
    }
    return hash;
}


} // portal
