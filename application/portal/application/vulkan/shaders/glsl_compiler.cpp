//
// Created by Jonatan Nevo on 09/03/2025.
//

#include "glsl_compiler.h"

namespace portal::vulkan
{
inline shaderc_shader_kind get_shader_kind(const vk::ShaderStageFlagBits stage)
{
    switch (stage)
    {
    case vk::ShaderStageFlagBits::eVertex:
        return shaderc_shader_kind::shaderc_vertex_shader;
    case vk::ShaderStageFlagBits::eTessellationControl:
        return shaderc_shader_kind::shaderc_tess_control_shader;
    case vk::ShaderStageFlagBits::eTessellationEvaluation:
        return shaderc_shader_kind::shaderc_tess_evaluation_shader;
    case vk::ShaderStageFlagBits::eGeometry:
        return shaderc_shader_kind::shaderc_geometry_shader;
    case vk::ShaderStageFlagBits::eFragment:
        return shaderc_shader_kind::shaderc_fragment_shader;
    case vk::ShaderStageFlagBits::eCompute:
        return shaderc_shader_kind::shaderc_compute_shader;
    case vk::ShaderStageFlagBits::eRaygenKHR:
        return shaderc_shader_kind::shaderc_raygen_shader;
    case vk::ShaderStageFlagBits::eAnyHitKHR:
        return shaderc_shader_kind::shaderc_anyhit_shader;
    case vk::ShaderStageFlagBits::eClosestHitKHR:
        return shaderc_shader_kind::shaderc_closesthit_shader;
    case vk::ShaderStageFlagBits::eMissKHR:
        return shaderc_shader_kind::shaderc_miss_shader;
    case vk::ShaderStageFlagBits::eIntersectionKHR:
        return shaderc_shader_kind::shaderc_intersection_shader;
    case vk::ShaderStageFlagBits::eCallableKHR:
        return shaderc_shader_kind::shaderc_callable_shader;
    case vk::ShaderStageFlagBits::eTaskEXT:
        return shaderc_shader_kind::shaderc_task_shader;
    case vk::ShaderStageFlagBits::eMeshEXT:
        return shaderc_shader_kind::shaderc_mesh_shader;
    default:
        return shaderc_shader_kind::shaderc_glsl_infer_from_source;
    }
}

void GLSLCompiler::set_target_environment(shaderc_target_env target, uint32_t version)
{
    GLSLCompiler::env_target_language = target;
    GLSLCompiler::env_target_language_version = version;
}

void GLSLCompiler::reset_target_environment()
{
    GLSLCompiler::env_target_language = shaderc_target_env::shaderc_target_env_default;
    GLSLCompiler::env_target_language_version = 0;
}

bool GLSLCompiler::compile_to_spirv(
    const vk::ShaderStageFlagBits stage,
    const ShaderSource& shader_source,
    const std::string& entry_point,
    const ShaderVariant& shader_variant,
    std::vector<std::uint32_t>& spirv,
    std::string& info_log
)
{
    const auto shader_kind = get_shader_kind(stage);
    shaderc::CompileOptions options;
    if (GLSLCompiler::env_target_language != shaderc_target_env_default)
        options.SetTargetEnvironment(env_target_language, env_target_language_version);
    options.SetGenerateDebugInfo();

    const shaderc::Compiler compiler;
    const auto preprocessed = compiler.PreprocessGlsl(
        shader_source.get_source(),
        shader_kind,
        shader_source.get_filename().c_str(),
        options
    );

    if (preprocessed.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        info_log = preprocessed.GetErrorMessage();
        return false;
    }

    auto result = compiler.CompileGlslToSpv(
        {preprocessed.begin(), preprocessed.end()},
        shader_kind,
        shader_source.get_filename().c_str(),
        entry_point.c_str(),
        options
    );

    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        info_log = result.GetErrorMessage();
        return false;
    }
    spirv = {result.begin(), result.end()};
    return true;
}
} // portal
