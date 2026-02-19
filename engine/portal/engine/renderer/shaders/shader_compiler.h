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

    /** @brief Processes parameters from variable layout */
    void process_parameters_from_variable_layout(ShaderReflection& reflection, slang::VariableLayoutReflection* var_layout, slang::ProgramLayout* program_layout);

    /** @brief Processes entry point parameters */
    void process_entry_point_parameters(ShaderReflection& reflection, slang::EntryPointLayout* entry_point_layout);

    /** @brief Ensures descriptor set exists and returns a reference to it */
    static shader_reflection::ShaderDescriptorSet& ensure_descriptor_set(ShaderReflection& reflection, size_t descriptor_set);

    /** @brief Creates and registers an image/sampler descriptor */
    static void add_image_descriptor(
        ShaderReflection& reflection,
        StringId name_id,
        DescriptorType type,
        ShaderStage stage,
        size_t descriptor_set,
        size_t binding_index,
        unsigned base_shape,
        slang::TypeLayoutReflection* type_layout
    );

    /** @brief Creates and registers a buffer descriptor */
    static void add_buffer_descriptor(
        ShaderReflection& reflection,
        StringId name_id,
        DescriptorType type,
        ShaderStage stage,
        size_t descriptor_set,
        size_t binding_index,
        slang::TypeLayoutReflection* element_type_layout
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

    /** @brief Processes push constant parameter */
    void process_push_constant_parameter(ShaderReflection& reflection, const char* name, slang::TypeLayoutReflection* type_layout, size_t offset) const;

    /** @brief Processes buffer uniforms */
    static void process_buffer_uniforms(
        shader_reflection::BufferDescriptor& buffer,
        slang::TypeLayoutReflection* type_layout,
        StringId buffer_name,
        size_t buffer_offset
    );

    /** @brief Gets image dimensions from resource shape */
    static size_t get_image_dimensions_from_shape(unsigned int base_shape);

    /** @brief Gets array size from type layout */
    static size_t get_array_size(slang::TypeLayoutReflection* type_layout);

    /** @brief Reflects a struct type's fields into a ReflectedStruct */
    static shader_reflection::ReflectedStruct reflect_struct_type(
        slang::TypeLayoutReflection* struct_layout,
        StringId struct_name
    );

private:
    ShaderStage current_stage;
    Slang::ComPtr<slang::IGlobalSession> global_session;
};
} // portal
