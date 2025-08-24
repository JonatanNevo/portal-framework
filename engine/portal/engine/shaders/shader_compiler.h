//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <filesystem>
#include <portal/core/buffer.h>

#include "slang-com-ptr.h"
#include "portal/engine/shaders/shader_types.h"
#include "portal/engine/strings/string_id.h"

namespace portal
{

struct CompiledShader
{
    Buffer code;
    ShaderReflection reflection;
};

class ShaderCompiler
{
public:
    struct CompileRequest
    {
        StringId name;
        std::filesystem::path shader_path;
        Buffer shader_data;
    };

public:
    ShaderCompiler();
    CompiledShader compile(const CompileRequest& request);

protected:
    ShaderReflection reflect_shader(slang::ProgramLayout* layout);
    static void populate_binding_points(ShaderReflection& reflection);

protected:
    struct ShaderDescriptorBuilder
    {
        std::vector<ShaderDescriptorBinding> bindings;
        StringId name;
        int set_index = -1;
    };

private:
    void add_global_scope_parameters(ShaderReflection& reflection, ShaderDescriptorBuilder& descriptor_builder, slang::ProgramLayout* layout);
    void add_entry_point_parameters(ShaderReflection& reflection, ShaderDescriptorBuilder& shader_descriptor_builder, slang::ProgramLayout* layout);
    void add_entry_point_parameters(
        ShaderReflection& reflection,
        ShaderDescriptorBuilder& shader_descriptor_builder,
        slang::EntryPointLayout* layout
        );

    void add_ranges(ShaderReflection& reflection, ShaderDescriptorBuilder& descriptor_builder, slang::TypeLayoutReflection* type_layout);
    void add_sub_object_ranges(ShaderReflection& reflection, slang::TypeLayoutReflection* type_layout);
    void add_sub_object_range(ShaderReflection& reflection, slang::TypeLayoutReflection* type_layout, size_t range_index);
    
    void add_descriptor_for_parameter_block(const StringId& name, ShaderReflection& reflection, slang::TypeLayoutReflection* type_layout);
    void add_ranges_for_parameter_block_element(
        ShaderReflection& reflection,
        ShaderDescriptorBuilder& descriptor_builder,
        const StringId& name,
        slang::VariableLayoutReflection* element_var
        );

    void add_push_constant_range(ShaderReflection& reflection, slang::TypeLayoutReflection* type_layout) const;

    void add_descriptor_ranges(ShaderDescriptorBuilder& descriptor_builder, slang::TypeLayoutReflection* type_layout) const;
    void add_descriptor_range(
        ShaderDescriptorBuilder& descriptor_builder,
        slang::TypeLayoutReflection* type_layout,
        int relative_set_index,
        size_t range_index
        ) const;
    static void populate_binding_with_field_type(slang::TypeLayoutReflection* var_type, ShaderDescriptorBinding& binding);

    void add_automatically_introduced_uniform_buffer(StringId name, ShaderDescriptorBuilder& descriptor_builder, slang::VariableLayoutReflection* variable) const;

    static void start_building_descriptor(ShaderReflection& reflection, ShaderDescriptorBuilder& descriptor_builder);
    static void finish_building_descriptor(ShaderReflection& reflection, const ShaderDescriptorBuilder& descriptor_builder);
    static void filter_out_empty_descriptors(ShaderReflection& builder);

    static void finish_building_layout(ShaderReflection& reflection);

private:
    ShaderStage current_stage;
    Slang::ComPtr<slang::IGlobalSession> global_session;

};

} // portal
