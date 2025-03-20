//
// Created by Jonatan Nevo on 09/03/2025.
//

#pragma once
#include <shaderc/shaderc.hpp>

#include "portal/application/vulkan/shaders/shader_module.h"

namespace portal::vulkan
{
class GLSLCompiler
{
private:
    static shaderc_target_env env_target_language;
    static uint32_t env_target_language_version;

public:
    /**
     * @brief Set the shaderc target environment to translate to when generating code
     * @param target_language The language to translate to
     * @param target_language_version The version of the language to translate to
     */
    static void set_target_environment(shaderc_target_env target, uint32_t version);

    /**
     * @brief Reset the glslang target environment to the default values
     */
    static void reset_target_environment();

    /**
     * @brief Compiles GLSL to SPIRV code
     * @param stage The Vulkan shader stage flag
     * @param shader_source The GLSL source code to be compiled
     * @param entry_point The entrypoint function name of the shader stage
     * @param shader_variant The shader variant
     * @param[out] spirv The generated SPIRV code
     * @param[out] info_log Stores any log messages during the compilation process
     */
    static bool compile_to_spirv(
        vk::ShaderStageFlagBits stage,
        const ShaderSource& shader_source,
        const std::string& entry_point,
        const ShaderVariant& shader_variant,
        std::vector<std::uint32_t>& spirv,
        std::string& info_log
    );
};
} // portal
