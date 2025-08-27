//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "shader_compiler.h"

#include "portal/engine/shaders/shader_types.h"

namespace portal
{

static auto logger = Log::get_logger("ShaderCompiler");

void diagnose_if_needed(const Slang::ComPtr<slang::IBlob>& diagnostics_blob)
{
    if (diagnostics_blob != nullptr)
    {
        const auto diagnostics = static_cast<const char*>(diagnostics_blob->getBufferPointer());
        LOGGER_TRACE("slang diagnostics: {}", diagnostics);
    }
}

ShaderStage to_shader_stage(const SlangStage stage)
{
    switch (stage)
    {
    case SLANG_STAGE_NONE:
        return ShaderStage::All;
    case SLANG_STAGE_VERTEX:
        return ShaderStage::Vertex;
    case SLANG_STAGE_GEOMETRY:
        return ShaderStage::Geometry;
    case SLANG_STAGE_FRAGMENT:
        return ShaderStage::Fragment;
    case SLANG_STAGE_COMPUTE:
        return ShaderStage::Compute;
    case SLANG_STAGE_RAY_GENERATION:
        return ShaderStage::RayGeneration;
    case SLANG_STAGE_INTERSECTION:
        return ShaderStage::Intersection;
    case SLANG_STAGE_ANY_HIT:
        return ShaderStage::AnyHit;
    case SLANG_STAGE_CLOSEST_HIT:
        return ShaderStage::ClosestHit;
    case SLANG_STAGE_MISS:
        return ShaderStage::Miss;
    case SLANG_STAGE_CALLABLE:
        return ShaderStage::Callable;
    case SLANG_STAGE_MESH:
        return ShaderStage::Mesh;
    default:
        return ShaderStage::All;
    }
}

DescriptorType to_descriptor_type(slang::BindingType binding_type)
{
    switch (binding_type)
    {
#define CASE(FROM, TO)             \
case slang::BindingType::FROM:     \
return portal::DescriptorType::TO

    CASE(Sampler, Sampler);
    CASE(CombinedTextureSampler, CombinedImageSampler);
    CASE(Texture, SampledImage);
    CASE(MutableTexture, StorageImage);
    CASE(TypedBuffer, UniformTexelBuffer);
    CASE(MutableTypedBuffer, StorageTexelBuffer);
    CASE(ConstantBuffer, UniformBuffer);
    CASE(RawBuffer, StorageBuffer);
    CASE(MutableRawBuffer, StorageBuffer);
    CASE(InputRenderTarget, InputAttachment);
    CASE(InlineUniformData, InlineUniformBlock);
    CASE(RayTracingAccelerationStructure, AccelerationStructure);

#undef CASE
    default:
        return DescriptorType::Unknown;
    }
}

reflection::PropertyContainerType to_property_container_type(slang::TypeLayoutReflection* type_layout)
{
    const auto kind = type_layout->getKind();
    switch (kind)
    {
    case slang::TypeReflection::Kind::None:
    case slang::TypeReflection::Kind::Struct:
    case slang::TypeReflection::Kind::Resource:
        return reflection::PropertyContainerType::object;

    case slang::TypeReflection::Kind::Array:
        return reflection::PropertyContainerType::array;
    case slang::TypeReflection::Kind::Vector:
        return reflection::PropertyContainerType::vector;
    case slang::TypeReflection::Kind::Matrix:
        return reflection::PropertyContainerType::matrix;
    case slang::TypeReflection::Kind::Scalar:
        return reflection::PropertyContainerType::scalar;

    default:
        LOGGER_WARN("Does not support reflection of non scalar fields");
    }
    return reflection::PropertyContainerType::invalid;
}

size_t get_element_number(slang::TypeLayoutReflection* type_layout)
{
    const auto container = to_property_container_type(type_layout);

    if (container == reflection::PropertyContainerType::array || container == reflection::PropertyContainerType::vector)
        return type_layout->getElementCount();
    if (container == reflection::PropertyContainerType::matrix)
        return type_layout->getRowCount() * type_layout->getColumnCount();

    return 1;
}

reflection::PropertyType to_property_type(slang::TypeLayoutReflection* type_layout)
{
    const auto kind = type_layout->getKind();
    switch (kind)
    {
    case slang::TypeReflection::Kind::None:
    case slang::TypeReflection::Kind::Struct:
    case slang::TypeReflection::Kind::Resource:
        return reflection::PropertyType::object;

    case slang::TypeReflection::Kind::Array:
    case slang::TypeReflection::Kind::Vector:
    case slang::TypeReflection::Kind::Matrix:
    case slang::TypeReflection::Kind::Scalar:
        break;

    default:
        LOGGER_WARN("Does not support reflection of non scalar fields");
        return reflection::PropertyType::invalid;
    }

    const auto scalar_type = type_layout->getScalarType();
    switch (scalar_type)
    {
    case slang::TypeReflection::None:
    case slang::TypeReflection::Void:
        return reflection::PropertyType::invalid;
    case slang::TypeReflection::Bool:
        return reflection::PropertyType::boolean;
    case slang::TypeReflection::Int32:
    case slang::TypeReflection::UInt32:
        return reflection::PropertyType::integer32;
    case slang::TypeReflection::Int64:
    case slang::TypeReflection::UInt64:
        return reflection::PropertyType::integer64;
    case slang::TypeReflection::Float32:
        return reflection::PropertyType::floating32;
    case slang::TypeReflection::Float64:
        return reflection::PropertyType::floating64;
    case slang::TypeReflection::Int8:
    case slang::TypeReflection::UInt8:
        return reflection::PropertyType::integer8;
    case slang::TypeReflection::Int16:
    case slang::TypeReflection::UInt16:
        return reflection::PropertyType::integer16;
    default:
        LOGGER_WARN("Invalid scalar type");
    }
    return reflection::PropertyType::invalid;
}

ShaderCompiler::ShaderCompiler() : current_stage(ShaderStage::All)
{
    slang::createGlobalSession(global_session.writeRef());
}

CompiledShader ShaderCompiler::compile(const CompileRequest& request)
{
    auto data = request.shader_data;
    if (data[data.size - 1] != '\0')
    {
        // Add null terminator for slang string interpretation
        data = Buffer::copy(request.shader_data.data, request.shader_data.size + 1);
        data[data.size - 1] = '\0';
    }

    // TODO: get this from request?
    slang::TargetDesc target_desc{
        .format = SLANG_SPIRV,
        .profile = global_session->findProfile("spirv_1_5"),
    };

    // TODO: get this from request
    std::vector<slang::CompilerOptionEntry> options{{
        {
            slang::CompilerOptionName::EmitSpirvDirectly,
            slang::CompilerOptionValue{slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
        },
        {
            slang::CompilerOptionName::VulkanUseEntryPointName,
            slang::CompilerOptionValue{slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
        },
        {
            slang::CompilerOptionName::MatrixLayoutColumn,
            // Try forcing column-major layout
            slang::CompilerOptionValue{slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
        }
    }};

    std::vector<slang::PreprocessorMacroDesc> macros = {
        {"MATERIAL_METALLICROUGHNESS", "1"}
    };

    auto parent_path = request.shader_path.parent_path().string();

    const char* search_paths[] = {
        parent_path.c_str()
    };

    const slang::SessionDesc session_desc{
        .targets = &target_desc,
        .targetCount = 1,
        .searchPaths = search_paths,
        .searchPathCount = std::size(search_paths),
        .preprocessorMacros = macros.data(),
        .preprocessorMacroCount = static_cast<uint32_t>(macros.size()),
        .compilerOptionEntries = options.data(),
        .compilerOptionEntryCount = static_cast<uint32_t>(options.size())
    };

    Slang::ComPtr<slang::ISession> session;
    global_session->createSession(session_desc, session.writeRef());

    Slang::ComPtr<slang::IModule> module;
    {
        Slang::ComPtr<slang::IBlob> diagnostics_blob;
        module = session->loadModuleFromSourceString(
            request.name.string.data(),
            request.shader_path.string().c_str(),
            data.as<const char*>(),
            diagnostics_blob.writeRef()
            );
        diagnose_if_needed(diagnostics_blob);
        if (!module)
        {
            LOGGER_ERROR("Failed to load shader: {}", request.name);
            return {};
        }
    }

    // Instead of looking for hardcoded entry points, iterate through all available ones
    auto entry_point_count = module->getDefinedEntryPointCount();
    LOGGER_TRACE("Found {} entry points in shader", entry_point_count);

    std::vector<Slang::ComPtr<slang::IEntryPoint>> entry_points;
    std::vector<slang::IComponentType*> component_types = {module};

    for (int i = 0; i < entry_point_count; i++)
    {
        auto& entry_point = entry_points.emplace_back();
        {
            Slang::ComPtr<slang::IBlob> diagnostics_blob;
            module->getDefinedEntryPoint(i, entry_point.writeRef());
            diagnose_if_needed(diagnostics_blob);
            if (!entry_point)
            {
                LOGGER_ERROR("Entry point {} is null", i);
                continue;
            }
        }

        const auto reflection = entry_point->getFunctionReflection();
        if (!reflection)
        {
            LOGGER_ERROR("Entry point {} has no reflection", i);
            entry_points.pop_back(); // Remove invalid entry point
            continue;
        }

        auto entry_point_name = reflection->getName();
        LOGGER_TRACE("found entry point: {}", entry_point_name);
        component_types.push_back(entry_point);
    }

    slang::IComponentType* composed_program;
    {
        Slang::ComPtr<slang::IBlob> diagnostics_blob;
        const auto result = session->createCompositeComponentType(
            component_types.data(),
            component_types.size(),
            &composed_program,
            diagnostics_blob.writeRef()
            );
        diagnose_if_needed(diagnostics_blob);

        if (SLANG_FAILED(result))
        {
            LOGGER_ERROR("Failed to create composite component type");
            return {};
        }
    }

    Slang::ComPtr<slang::IComponentType> linked_program;
    {
        Slang::ComPtr<slang::IBlob> diagnostics_blob;
        const auto result = composed_program->link(
            linked_program.writeRef(),
            diagnostics_blob.writeRef()
            );
        diagnose_if_needed(diagnostics_blob);

        if (SLANG_FAILED(result))
        {
            LOGGER_ERROR("Failed to link program");
            return {};
        }
    }

    if (!linked_program)
    {
        LOGGER_ERROR("linked_program is null");
        return {};
    }

    Slang::ComPtr<slang::IBlob> spirv_code;
    {
        Slang::ComPtr<slang::IBlob> diagnostics_blob;

        LOGGER_TRACE("Getting target code from linked program");
        const auto result = linked_program->getTargetCode(0, spirv_code.writeRef(), diagnostics_blob.writeRef());
        diagnose_if_needed(diagnostics_blob);

        if (SLANG_FAILED(result))
        {
            LOGGER_ERROR("Failed to compile program");
            return {};
        }

        if (!spirv_code)
        {
            LOGGER_ERROR("SPIRV code generation failed - spirv_code is null");
            return {};
        }

        LOGGER_TRACE("Successfully generated SPIRV code of size: {}", spirv_code->getBufferSize());
    }

    const auto layout = linked_program->getLayout();
    const auto reflection = reflect_shader(layout);
    return {
        .code = Buffer::copy(spirv_code->getBufferPointer(), spirv_code->getBufferSize()),
        .reflection = reflection
    };
}

ShaderReflection ShaderCompiler::reflect_shader(slang::ProgramLayout* layout)
{
    Slang::ComPtr<slang::IBlob> json_blob;
    layout->toJson(json_blob.writeRef());
    Buffer json_data = Buffer::copy(json_blob->getBufferPointer(), json_blob->getBufferSize());
    LOGGER_TRACE(json_data.as_string());

    ShaderReflection reflection;
    ShaderDescriptorBuilder global_descriptor_builder;
    start_building_descriptor(reflection, global_descriptor_builder);
    global_descriptor_builder.name = INVALID_STRING_ID;

    add_global_scope_parameters(reflection, global_descriptor_builder, layout);

    add_entry_point_parameters(reflection, global_descriptor_builder, layout);

    finish_building_descriptor(reflection, global_descriptor_builder);
    finish_building_layout(reflection);

    populate_binding_points(reflection);
    return reflection;
}

void ShaderCompiler::populate_binding_points(ShaderReflection& reflection)
{
    for (size_t layout_index = 0; layout_index < reflection.layouts.size(); ++layout_index)
    {
        auto& layout = reflection.layouts[layout_index];
        for (size_t binding_index = 0; binding_index < layout.bindings.size(); ++binding_index)
        {
            auto& binding = layout.bindings[binding_index];

            if (!binding.fields.empty())
            {
                for (auto& [layout_name, field_layout] : binding.fields)
                {
                    auto name = STRING_ID(fmt::format("{}.{}", binding.name.string, layout_name.string));
                    reflection.bind_points[name] = {
                        .name = field_layout.name,
                        .layout_index = layout_index,
                        .binding_index = binding_index,
                        .field_name = layout_name
                    };
                }
            }
            else
            {
                StringId name;
                if (layout.name == INVALID_STRING_ID)
                    name = binding.name;
                else
                    name = STRING_ID(fmt::format("{}.{}", layout.name.string, binding.name.string));

                reflection.bind_points[name] = {
                    .name = binding.name,
                    .layout_index = layout_index,
                    .binding_index = binding_index
                };
            }
        }

    }
}

void ShaderCompiler::add_global_scope_parameters(
    ShaderReflection& reflection,
    ShaderDescriptorBuilder& descriptor_builder,
    slang::ProgramLayout* layout
    )
{
    current_stage = ShaderStage::All;
    add_ranges_for_parameter_block_element(reflection, descriptor_builder, INVALID_STRING_ID, layout->getGlobalParamsVarLayout());
}

void ShaderCompiler::add_entry_point_parameters(
    ShaderReflection& reflection,
    ShaderDescriptorBuilder& shader_descriptor_builder,
    slang::ProgramLayout* layout
    )
{
    const size_t entry_point_count = layout->getEntryPointCount();
    for (size_t i = 0; i < entry_point_count; ++i)
    {
        const auto entry_point_layout = layout->getEntryPointByIndex(i);
        add_entry_point_parameters(reflection, shader_descriptor_builder, entry_point_layout);
    }
}

void ShaderCompiler::add_entry_point_parameters(
    ShaderReflection& reflection,
    ShaderDescriptorBuilder& shader_descriptor_builder,
    slang::EntryPointLayout* layout
    )
{
    current_stage = to_shader_stage(layout->getStage());
    reflection.entry_points[current_stage] = layout->getName();
    auto layout_name = STRING_ID(layout->getName());
    add_ranges_for_parameter_block_element(reflection, shader_descriptor_builder, layout_name, layout->getVarLayout());
}

void ShaderCompiler::add_ranges(
    ShaderReflection& reflection,
    ShaderDescriptorBuilder& descriptor_builder,
    slang::TypeLayoutReflection* type_layout
    )
{
    add_descriptor_ranges(descriptor_builder, type_layout);
    add_sub_object_ranges(reflection, type_layout);
}

void ShaderCompiler::add_sub_object_ranges(ShaderReflection& reflection, slang::TypeLayoutReflection* type_layout)
{
    const size_t sub_object_range_count = type_layout->getSubObjectRangeCount();
    for (size_t i = 0; i < sub_object_range_count; ++i)
    {
        add_sub_object_range(reflection, type_layout, i);
    }
}

void ShaderCompiler::add_sub_object_range(ShaderReflection& reflection, slang::TypeLayoutReflection* type_layout, const size_t range_index)
{
    const auto binding_range_index = type_layout->getSubObjectRangeBindingRangeIndex(range_index);
    const auto binding_type = type_layout->getBindingRangeType(binding_range_index);

    const auto inner_type = type_layout->getBindingRangeLeafTypeLayout(binding_range_index);
    const auto var = type_layout->getBindingRangeLeafVariable(range_index);
    const StringId name = var->getName() == nullptr ? INVALID_STRING_ID : STRING_ID(var->getName());

    switch (binding_type)
    {
    default:
        LOGGER_WARN("Found skippable binding type");
        return;

    case slang::BindingType::ParameterBlock:
    {
        add_descriptor_for_parameter_block(name, reflection, inner_type);
        break;
    }
    case slang::BindingType::PushConstant:
    {
        add_push_constant_range(reflection, inner_type);
        break;
    }
    }
}

void ShaderCompiler::add_descriptor_for_parameter_block(const StringId& name, ShaderReflection& reflection, slang::TypeLayoutReflection* type_layout)
{
    ShaderDescriptorBuilder descriptor_builder;
    start_building_descriptor(reflection, descriptor_builder);
    descriptor_builder.name = name;

    add_ranges_for_parameter_block_element(reflection, descriptor_builder, name, type_layout->getElementVarLayout());
    finish_building_descriptor(reflection, descriptor_builder);
}

void ShaderCompiler::add_ranges_for_parameter_block_element(
    ShaderReflection& reflection,
    ShaderDescriptorBuilder& descriptor_builder,
    const StringId& name,
    slang::VariableLayoutReflection* element_var
    )
{
    const auto element_type = element_var->getTypeLayout();
    if (element_type->getSize() > 0)
    {
        add_automatically_introduced_uniform_buffer(name, descriptor_builder, element_var);
    }

    // Once we have accounted for the possibility of an implicitly-introduced
    // constant buffer, we can move on and add bindings based on whatever
    // non-ordinary data (textures, buffers, etc.) is in the element type:
    //
    add_ranges(reflection, descriptor_builder, element_type);
}

void ShaderCompiler::add_push_constant_range(ShaderReflection& reflection, slang::TypeLayoutReflection* type_layout) const
{
    const auto element_type_layout = type_layout->getElementTypeLayout();
    const auto element_size = element_type_layout->getSize();

    if (element_size == 0)
        return;

    const auto name = type_layout->getName();
    reflection.push_constants.push_back(
        {
            .name = name == nullptr ? INVALID_STRING_ID : STRING_ID(name),
            .stage = current_stage,
            .size = element_size,
            .offset = 0,
        }
        );
}

void ShaderCompiler::add_descriptor_ranges(ShaderDescriptorBuilder& descriptor_builder, slang::TypeLayoutReflection* type_layout) const
{
    int relative_set_index = 0;
    size_t range_count = type_layout->getDescriptorSetDescriptorRangeCount(relative_set_index);

    for (size_t range_index = 0; range_index < range_count; ++range_index)
    {
        add_descriptor_range(descriptor_builder, type_layout, relative_set_index, range_index);
    }
}

void ShaderCompiler::add_descriptor_range(
    ShaderDescriptorBuilder& descriptor_builder,
    slang::TypeLayoutReflection* type_layout,
    const int relative_set_index,
    const size_t range_index
    ) const
{
    const auto binding_type = type_layout->getDescriptorSetDescriptorRangeType(relative_set_index, range_index);
    const auto descriptor_count = type_layout->getDescriptorSetDescriptorRangeDescriptorCount(relative_set_index, range_index);
    const auto descriptor_type = type_layout->getBindingRangeLeafTypeLayout(range_index);
    const auto descriptor_variable = type_layout->getBindingRangeLeafVariable(range_index);

    const auto binding_index = descriptor_builder.bindings.size();
    auto name = descriptor_variable->getName() == nullptr ? INVALID_STRING_ID : STRING_ID(descriptor_variable->getName());

    ShaderDescriptorBinding binding = {
        .stage = current_stage,
        .binding_index = binding_index,
        .type = to_descriptor_type(binding_type),
        .descriptor_count = static_cast<size_t>(descriptor_count),
        .name = name
    };

    switch (binding_type)
    {
    default:
        break;

    case slang::BindingType::ConstantBuffer:
    {
        populate_binding_with_field_type(descriptor_type->getElementTypeLayout(), binding);
        break;
    }

    case slang::BindingType::PushConstant:
        return;
    }

    LOGGER_TRACE("Descriptor range: \"{}\" ({}) [{}]", name, binding_index, to_descriptor_type(binding_type));
    descriptor_builder.bindings.push_back(binding);
}

void ShaderCompiler::populate_binding_with_field_type(slang::TypeLayoutReflection* var_type, ShaderDescriptorBinding& binding)
{
    for (unsigned int i = 0; i < var_type->getFieldCount(); i++)
    {
        auto* field = var_type->getFieldByIndex(i);
        const auto field_layout = field->getTypeLayout();

        FieldLayout layout{
            .name = STRING_ID(field->getName() == nullptr ? "" : field->getName()),
            .offset = field->getOffset(),
            .size = field_layout->getSize()
        };

        if (layout.size != 0)
        {
            layout.property = {
                .type = to_property_type(field_layout),
                .container_type = to_property_container_type(field_layout),
                .elements_number = get_element_number(field_layout)
            };

            LOGGER_TRACE(
                "\tField: \"{}\" [{}] size {} offset {}",
                layout.name,
                layout.property,
                layout.size,
                layout.offset
                );
            binding.fields[layout.name] = layout;
        }
    }
}

void ShaderCompiler::add_automatically_introduced_uniform_buffer(
    StringId name,
    ShaderDescriptorBuilder& descriptor_builder,
    slang::VariableLayoutReflection* variable
    ) const
{
    const auto binding_index = descriptor_builder.bindings.size();

    LOGGER_TRACE("Uniform buffer: {} ({})", name, binding_index);
    ShaderDescriptorBinding binding = {
        .stage = current_stage,
        .binding_index = binding_index,
        .type = DescriptorType::UniformBuffer,
        .descriptor_count = 1,
        .name = name
    };

    const auto var_type = variable->getTypeLayout();
    populate_binding_with_field_type(var_type, binding);

    descriptor_builder.bindings.push_back(binding);
}

void ShaderCompiler::start_building_descriptor(ShaderReflection& reflection, ShaderDescriptorBuilder& descriptor_builder)
{
    descriptor_builder.set_index = static_cast<int>(reflection.layouts.size());
    reflection.layouts.push_back({});
}

void ShaderCompiler::finish_building_descriptor(ShaderReflection& reflection, const ShaderDescriptorBuilder& descriptor_builder)
{
    if (descriptor_builder.bindings.empty())
        return;

    reflection.layouts.push_back({.name = descriptor_builder.name, .bindings = descriptor_builder.bindings});
}

void ShaderCompiler::filter_out_empty_descriptors(ShaderReflection& builder)
{
    std::vector<ShaderDescriptorLayout> filtered_layouts;
    for (auto descriptor_layout : builder.layouts)
    {
        if (descriptor_layout.bindings.empty())
            continue;

        filtered_layouts.push_back(descriptor_layout);
    }
    std::swap(builder.layouts, filtered_layouts);
}

void ShaderCompiler::finish_building_layout(ShaderReflection& reflection)
{
    filter_out_empty_descriptors(reflection);
}

} // portal
