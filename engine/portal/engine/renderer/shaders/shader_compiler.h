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
/**
 * @struct CompiledShader
 * @brief Compiled shader bytecode and reflection data
 */
struct CompiledShader
{
    Buffer code = nullptr;
    ShaderReflection reflection{};
};

/**
 * @class ShaderCompiler
 * @brief Compiles shaders using Slang and extracts reflection metadata
 *
 * Processes shader source with defines, compiles to bytecode, and reflects
 * descriptor sets, push constants, and resource bindings.
 */
class ShaderCompiler
{
public:
    /**
     * @struct CompileRequest
     * @brief Shader compilation request parameters
     */
    struct CompileRequest
    {
        StringId name;
        std::filesystem::path shader_path;
        std::filesystem::path engine_shader_path;
        Buffer shader_data;
        std::vector<ShaderDefine> defines;
        std::vector<ShaderStaticConstants> static_constants;
    };

public:
    ShaderCompiler();

    /**
     * @brief Compiles shader and extracts reflection
     * @param request Compilation parameters
     * @return Compiled bytecode and reflection data
     */
    CompiledShader compile(const CompileRequest& request);

private:
    /** @brief Extracts reflection from Slang program layout */
    ShaderReflection reflect_shader(slang::ProgramLayout* layout);

    /** @brief Processes variable layout reflection */
    void process_variable_layout(ShaderReflection& reflection, slang::VariableLayoutReflection* var_layout, slang::ProgramLayout* program_layout);

    /** @brief Processes parameters from variable layout */
    void process_parameters_from_variable_layout(ShaderReflection& reflection, slang::VariableLayoutReflection* var_layout, slang::ProgramLayout* program_layout);

    /** @brief Processes entry point parameters */
    void process_entry_point_parameters(ShaderReflection& reflection, slang::EntryPointLayout* entry_point_layout);

    /** @brief Processes constant buffer parameter */
    static void process_constant_buffer_parameter(
        ShaderReflection& reflection,
        const char* name,
        slang::TypeLayoutReflection* type_layout,
        int space,
        size_t binding_index
    );
    static void add_combined_texture_sampler_dec(
        ShaderReflection& reflection,
        const char* name,
        size_t descriptor_set,
        shader_reflection::ShaderDescriptorSet& desc_set,
        size_t resource_binding_counter,
        char const* field_name,
        slang::TypeLayoutReflection* field_layout,
        unsigned base_shape
    );
    static void add_separate_texture_dec(
        ShaderReflection& reflection,
        const char* name,
        size_t descriptor_set,
        shader_reflection::ShaderDescriptorSet& desc_set,
        size_t resource_binding_counter,
        char const* field_name,
        slang::TypeLayoutReflection* field_layout,
        unsigned base_shape
    );

    /** @brief Processes parameter block parameter */
    static void process_parameter_block_parameter(
        ShaderReflection& reflection,
        const char* name,
        slang::TypeLayoutReflection* type_layout,
        int space,
        size_t binding_index,
        slang::ProgramLayout* program_layout
    );

    /** @brief Processes combined texture-sampler parameter */
    void process_combined_texture_sampler_parameter(
        ShaderReflection& reflection,
        const char* name,
        slang::TypeLayoutReflection* type_layout,
        int space,
        size_t binding_index,
        unsigned int base_shape
    ) const;

    /** @brief Processes resource parameter (texture, image, buffer) */
    void process_resource_parameter(
        ShaderReflection& reflection,
        const char* name,
        slang::TypeLayoutReflection* type_layout,
        int space,
        size_t binding_index,
        unsigned int resource_shape
    ) const;

    /** @brief Processes push constant parameter */
    void process_push_constant_parameter(ShaderReflection& reflection, const char* name, slang::TypeLayoutReflection* type_layout, size_t offset) const;

    /** @brief Processes buffer uniforms */
    static void process_buffer_uniforms(
        shader_reflection::BufferDescriptor& buffer,
        slang::TypeLayoutReflection* type_layout,
        StringId buffer_name,
        size_t buffer_offset
    );

    /** @brief Gets image dimensions from type layout */
    static size_t get_image_dimensions(slang::TypeLayoutReflection* type_layout);

    /** @brief Gets image dimensions from resource shape */
    static size_t get_image_dimensions_from_shape(unsigned int base_shape);

    /** @brief Gets array size from type layout */
    static size_t get_array_size(slang::TypeLayoutReflection* type_layout);

private:
    ShaderStage current_stage;
    Slang::ComPtr<slang::IGlobalSession> global_session;
};
} // portal
