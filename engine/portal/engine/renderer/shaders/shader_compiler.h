//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <filesystem>

#include "slang-com-ptr.h"
#include "portal/engine/renderer/shaders/shader_types.h"
#include "portal/core/strings/string_id.h"

namespace portal::renderer
{
struct CompiledShader
{
    Buffer code = nullptr;
    ShaderReflection reflection{};
};

class ShaderCompiler
{
public:
    struct CompileRequest
    {
        StringId name;
        std::filesystem::path shader_path;
        Buffer shader_data;
        std::vector<ShaderDefine> defines;
    };

public:
    ShaderCompiler();
    CompiledShader compile(const CompileRequest& request);

private:
    ShaderReflection reflect_shader(slang::ProgramLayout* layout);

    void process_variable_layout(ShaderReflection& reflection, slang::VariableLayoutReflection* var_layout);
    void process_parameters_from_variable_layout(ShaderReflection& reflection, slang::VariableLayoutReflection* var_layout);
    void process_entry_point_parameters(ShaderReflection& reflection, slang::EntryPointLayout* entry_point_layout);
    void process_constant_buffer_parameter(
        ShaderReflection& reflection,
        const char* name,
        slang::TypeLayoutReflection* type_layout,
        int space,
        size_t binding_index
    );
    void process_parameter_block_parameter(
        ShaderReflection& reflection,
        const char* name,
        slang::TypeLayoutReflection* type_layout,
        int space,
        size_t binding_index
    );
    void process_combined_texture_sampler_parameter(
        ShaderReflection& reflection,
        const char* name,
        slang::TypeLayoutReflection* type_layout,
        int space,
        size_t binding_index,
        unsigned int base_shape
    ) const;
    void process_resource_parameter(
        ShaderReflection& reflection,
        const char* name,
        slang::TypeLayoutReflection* type_layout,
        int space,
        size_t binding_index,
        unsigned int resource_shape
    );
    void process_push_constant_parameter(ShaderReflection& reflection, const char* name, slang::TypeLayoutReflection* type_layout, size_t offset);
    void process_buffer_uniforms(
        shader_reflection::BufferDescriptor& buffer,
        slang::TypeLayoutReflection* type_layout,
        StringId buffer_name,
        size_t buffer_offset
    );
    static size_t get_image_dimensions(slang::TypeLayoutReflection* type_layout);
    static size_t get_image_dimensions_from_shape(unsigned int base_shape);
    static size_t get_array_size(slang::TypeLayoutReflection* type_layout);

private:
    ShaderStage current_stage;
    Slang::ComPtr<slang::IGlobalSession> global_session;
};
} // portal
