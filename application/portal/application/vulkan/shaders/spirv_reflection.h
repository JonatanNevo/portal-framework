//
// Created by Jonatan Nevo on 04/03/2025.
//

#pragma once

#include <spirv_glsl.hpp>
#include "shader_module.h"

namespace portal::vulkan
{
/// Generate a list of shader resource based on SPIRV reflection code, and provided ShaderVariant
class SPIRVReflection
{
public:
    /// @brief Reflects shader resources from SPIRV code
    /// @param stage The Vulkan shader stage flag
    /// @param spirv The SPIRV code of shader
    /// @param[out] resources The list of reflected shader resources
    /// @param variant ShaderVariant used for reflection to specify the size of the runtime arrays in Storage Buffers
    static bool reflect_shader_resources(
        vk::ShaderStageFlagBits stage,
        const std::vector<uint32_t>& spirv,
        std::vector<ShaderResource>& resources,
        const ShaderVariant& variant
    );

private:
    static void parse_shader_resources(
        const spirv_cross::Compiler& compiler,
        vk::ShaderStageFlagBits stage,
        std::vector<ShaderResource>& resources,
        const ShaderVariant& variant
    );

    static void parse_push_constants(
        const spirv_cross::Compiler& compiler,
        vk::ShaderStageFlagBits stage,
        std::vector<ShaderResource>& resources,
        const ShaderVariant& variant
    );

    static void parse_specialization_constants(
        const spirv_cross::Compiler& compiler,
        vk::ShaderStageFlagBits stage,
        std::vector<ShaderResource>& resources,
        const ShaderVariant& variant
    );
};
} // portal
