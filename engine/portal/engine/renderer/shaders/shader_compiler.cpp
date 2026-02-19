//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "shader_compiler.h"

#include "shader_types.h"
#include "portal/core/strings/string_utils.h"

namespace portal::renderer
{
static auto logger = Log::get_logger("ShaderCompiler");

void diagnose_if_needed(const Slang::ComPtr<slang::IBlob>& diagnostics_blob)
{
    if (diagnostics_blob != nullptr)
    {
        const auto diagnostics = static_cast<const char*>(diagnostics_blob->getBufferPointer());
        LOGGER_WARN("slang diagnostics: {}", diagnostics);
    }
}

std::string format_property(reflection::Property prop)
{
    switch (prop.container_type)
    {
    case reflection::PropertyContainerType::scalar:
        return std::string(to_string(prop.type));
    case reflection::PropertyContainerType::array:
        if (prop.type == reflection::PropertyType::object)
            return fmt::format("{}[{}]", prop.value.as_string(), prop.elements_number);
        return fmt::format("{}[{}]", prop.type, prop.elements_number);
    case reflection::PropertyContainerType::string:
    case reflection::PropertyContainerType::null_term_string:
        return "string";
    case reflection::PropertyContainerType::vector:
        return fmt::format("vec{}<{}>", prop.elements_number, prop.type);
    case reflection::PropertyContainerType::matrix:
        return fmt::format("mat{0}x{0}<{1}>", floor(sqrt(prop.elements_number)), prop.type);
    case reflection::PropertyContainerType::object:
    case reflection::PropertyContainerType::invalid:
        break;
    }
    return "invalid";
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
return portal::renderer::DescriptorType::TO

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

    if (container == reflection::PropertyContainerType::array || container ==
        reflection::PropertyContainerType::vector)
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
    LOGGER_DEBUG("Compiling shader: {}", request.name);
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
    std::vector<slang::CompilerOptionEntry> options{
        {
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
        }
    };

    // Global macros
    std::vector<slang::PreprocessorMacroDesc> macros = {
        // {"MATERIAL_METALLICROUGHNESS", "1"},
        {"HAS_NORMAL_VEC3", "1"},
        {"HAS_COLOR_0_VEC4", "1"},
        {"HAS_TEXCOORD_0_VEC2", "1"}
    };

    // Request macros
    for (const auto& [name, value] : request.defines)
    {
        macros.push_back({.name = name.c_str(), .value = value.c_str()});
        LOGGER_TRACE("Adding macro: {} = {}", name, value);
    }

    auto parent_path = request.shader_path.parent_path().string();
    auto engine_path = request.engine_shader_path.generic_string();

    const char* search_paths[] = {
        parent_path.c_str(),
        engine_path.c_str(),
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

    std::vector<slang::IComponentType*> component_types{};
    if (!request.static_constants.empty())
    {
        std::string consts_module_string;
        for (const auto& [name, type, value] : request.static_constants)
        {
            consts_module_string += fmt::format("\npublic export const static  {} {} = {};", type, name, value);
        }

        slang::IModule* consts_module;
        {
            Slang::ComPtr<slang::IBlob> diagnostics_blob;
            consts_module = session->loadModuleFromSourceString(
                "consts",
                "consts.slang",
                consts_module_string.c_str(),
                diagnostics_blob.writeRef()
            );
            diagnose_if_needed(diagnostics_blob);
            if (!consts_module)
            {
                LOGGER_ERROR("Failed to load consts module: {}", request.name);
                return {};
            }
        }
        component_types.push_back(consts_module);
    }

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
    LOGGER_DEBUG("Found {} entry points in shader", entry_point_count);

    std::vector<Slang::ComPtr<slang::IEntryPoint>> entry_points;
    component_types.emplace_back(module);

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
        LOGGER_DEBUG("found entry point: {}", entry_point_name);
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
    // // Keep this for debug
    // Slang::ComPtr<slang::IBlob> json_blob;
    // layout->toJson(json_blob.writeRef());
    // Buffer json_data = Buffer::copy(json_blob->getBufferPointer(), json_blob->getBufferSize());
    // LOGGER_TRACE(json_data.as_string());

    LOGGER_TRACE("===========================");
    LOGGER_TRACE(" Slang Shader Reflection");
    LOGGER_TRACE("===========================");

    ShaderReflection reflection;

    const auto global_params = layout->getGlobalParamsVarLayout();
    if (global_params)
        process_parameters_from_variable_layout(reflection, global_params, layout);

    // Process each entry point
    const size_t entry_point_count = layout->getEntryPointCount();
    for (size_t i = 0; i < entry_point_count; ++i)
    {
        const auto entry_point_layout = layout->getEntryPointByIndex(i);
        current_stage = to_shader_stage(entry_point_layout->getStage());

        // Process entry point parameters for push constants only
        process_entry_point_parameters(reflection, entry_point_layout);
        reflection.stages.push_back({.stage = current_stage, .entry_point = entry_point_layout->getName()});
    }

    LOGGER_TRACE("===========================");

    return reflection;
}

shader_reflection::ShaderDescriptorSet& ShaderCompiler::ensure_descriptor_set(ShaderReflection& reflection, size_t descriptor_set)
{
    if (descriptor_set >= reflection.descriptor_sets.size())
        reflection.descriptor_sets.resize(descriptor_set + 1);
    return reflection.descriptor_sets[descriptor_set];
}

void ShaderCompiler::add_image_descriptor(
    ShaderReflection& reflection,
    StringId name_id,
    DescriptorType type,
    ShaderStage stage,
    size_t descriptor_set,
    size_t binding_index,
    unsigned base_shape,
    slang::TypeLayoutReflection* type_layout
)
{
    auto& desc_set = ensure_descriptor_set(reflection, descriptor_set);

    shader_reflection::ImageSamplerDescriptor image;
    image.type = type;
    image.stage = stage;
    image.binding_point = binding_index;
    image.descriptor_set = descriptor_set;
    image.name = name_id;
    image.dimensions = get_image_dimensions_from_shape(base_shape);
    image.array_size = get_array_size(type_layout);

    if (type == DescriptorType::CombinedImageSampler)
        desc_set.image_samplers[binding_index] = image;
    else
        desc_set.images[binding_index] = image;

    reflection.resources[name_id] = shader_reflection::ShaderResourceDeclaration{
        .name = name_id,
        .type = type,
        .set = descriptor_set,
        .binding_index = binding_index,
        .count = get_array_size(type_layout)
    };

    LOGGER_TRACE("Image Descriptor:");
    LOGGER_TRACE("  {} ({}, {})", name_id.string, descriptor_set, binding_index);
    LOGGER_TRACE("  Dimensions: {}D", image.dimensions);
    LOGGER_TRACE("  Array Size: {}", image.array_size);
    LOGGER_TRACE("-------------------");
}

void ShaderCompiler::add_buffer_descriptor(
    ShaderReflection& reflection,
    StringId name_id,
    DescriptorType type,
    ShaderStage stage,
    size_t descriptor_set,
    size_t binding_index,
    slang::TypeLayoutReflection* element_type_layout
)
{
    auto& desc_set = ensure_descriptor_set(reflection, descriptor_set);

    const auto buffer_size = element_type_layout->getSize();

    shader_reflection::BufferDescriptor buffer;
    buffer.type = type;
    buffer.stage = stage;
    buffer.binding_point = binding_index;
    buffer.name = name_id;
    buffer.size = buffer_size;
    buffer.offset = 0;
    buffer.range = buffer_size;
    process_buffer_uniforms(buffer, element_type_layout, name_id, 0);

    if (type == DescriptorType::UniformBuffer)
        desc_set.uniform_buffers[binding_index] = buffer;
    else
        desc_set.storage_buffers[binding_index] = buffer;

    reflection.resources[name_id] = shader_reflection::ShaderResourceDeclaration{
        .name = name_id,
        .type = type,
        .set = descriptor_set,
        .binding_index = binding_index,
        .count = 1
    };

    LOGGER_TRACE("Buffer Descriptor:");
    LOGGER_TRACE("  {} ({}, {})", name_id.string, descriptor_set, binding_index);
    LOGGER_TRACE("  Size: {}", buffer_size);
    LOGGER_TRACE("  Fields:");
    for (const auto& [fst, snd] : buffer.uniforms)
        LOGGER_TRACE("    {}: {}", fst.string, format_property(snd.property));
    LOGGER_TRACE("-------------------");
}

void ShaderCompiler::process_parameters_from_variable_layout(
    ShaderReflection& reflection,
    slang::VariableLayoutReflection* var_layout,
    slang::ProgramLayout* program_layout
)
{
    const auto type_layout = var_layout->getTypeLayout();

    // Process each parameter/field in the variable layout
    const auto field_count = type_layout->getFieldCount();
    LOGGER_TRACE("Processing {} fields in variable layout", field_count);

    int parameter_block_space_counter = 0; // Assign sequential spaces to parameter blocks
    bool has_parameter_block = false;
    bool has_global_variables = false;

    for (unsigned int field_idx = 0; field_idx < field_count; ++field_idx)
    {
        const auto field = type_layout->getFieldByIndex(field_idx);
        if (!field) continue;

        const auto field_name = field->getName();
        if (!field_name) continue;

        const auto field_type_layout = field->getTypeLayout();
        if (!field_type_layout) continue;

        // Get binding information
        const auto space = field->getBindingSpace();
        const size_t binding_index = field->getBindingIndex();

        LOGGER_TRACE("Field '{}': space={}, binding={}", field_name, space, binding_index);

        // Process based on the type of resource
        const auto field_type = field_type_layout->getType();
        if (!field_type) continue;

        switch (field_type->getKind())
        {
        case slang::TypeReflection::Kind::ConstantBuffer:
            has_global_variables = true;
            add_buffer_descriptor(
                reflection,
                STRING_ID(field_name),
                DescriptorType::UniformBuffer,
                ShaderStage::All,
                static_cast<size_t>(space),
                binding_index,
                field_type_layout->getElementTypeLayout()
            );
            break;

        case slang::TypeReflection::Kind::Resource:
            {
                has_global_variables = true;
                const auto resource_shape = field_type->getResourceShape();
                const auto base_shape = resource_shape & SLANG_RESOURCE_BASE_SHAPE_MASK;

                if ((resource_shape & SLANG_TEXTURE_COMBINED_FLAG) != 0)
                {
                    add_image_descriptor(
                        reflection,
                        STRING_ID(field_name),
                        DescriptorType::CombinedImageSampler,
                        current_stage,
                        static_cast<size_t>(space),
                        binding_index,
                        base_shape,
                        field_type_layout
                    );
                }
                else if (base_shape & SLANG_STRUCTURED_BUFFER)
                {
                    add_buffer_descriptor(
                        reflection,
                        STRING_ID(field_name),
                        DescriptorType::StorageBuffer,
                        current_stage,
                        static_cast<size_t>(space),
                        binding_index,
                        field_type_layout->getElementTypeLayout()
                    );
                }
                else
                {
                    add_image_descriptor(
                        reflection,
                        STRING_ID(field_name),
                        DescriptorType::SampledImage,
                        current_stage,
                        static_cast<size_t>(space),
                        binding_index,
                        base_shape,
                        field_type_layout
                    );
                }
                break;
            }

        case slang::TypeReflection::Kind::Struct:
            {
                has_global_variables = true;
                // This might be a push constant buffer - check if it has uniform binding
                const auto uniform_offset = field->getOffset();
                if (uniform_offset != static_cast<size_t>(-1)) // Check for valid offset
                {
                    process_push_constant_parameter(reflection, field_name, field_type_layout, uniform_offset);
                }
                break;
            }
        case slang::TypeReflection::Kind::ParameterBlock:
            {
                has_parameter_block = true;
                // Assign our own sequential space instead of using what Slang reports
                // Parameter blocks always start at binding 0 for the uniform buffer
                process_parameter_block_parameter(reflection, field_name, field_type_layout, parameter_block_space_counter, 0, program_layout);
                parameter_block_space_counter++;
                break;
            }
        default:
            break;
        }
    }

    if (has_parameter_block && has_global_variables)
        LOGGER_WARN("Shader has both parameter blocks and global variables - this is not supported");
}

void ShaderCompiler::process_entry_point_parameters(ShaderReflection& reflection, slang::EntryPointLayout* entry_point_layout)
{
    if (!entry_point_layout) return;

    const auto var_layout = entry_point_layout->getVarLayout();
    if (!var_layout) return;

    const auto type_layout = var_layout->getTypeLayout();
    if (!type_layout) return;

    // Process entry point parameters looking specifically for push constants
    const auto param_count = entry_point_layout->getParameterCount();
    LOGGER_TRACE("Processing {} entry point parameters for stage {}", param_count, static_cast<int>(current_stage));

    size_t total_size = 0;
    int64_t range_offset = -1;
    for (unsigned param_idx = 0; param_idx < param_count; ++param_idx)
    {
        const auto param = entry_point_layout->getParameterByIndex(param_idx);
        if (!param) continue;

        const auto param_name = param->getName();
        if (!param_name) continue;

        // Skip vertex input and fragment output parameters
        const auto param_category = param->getCategory();

        // Only process uniform parameters (push constants)
        if (param_category == slang::ParameterCategory::Uniform)
        {
            const auto param_type_layout = param->getTypeLayout();
            if (!param_type_layout) continue;

            const auto offset = param->getOffset();
            const auto size = param_type_layout->getSize();

            // Only process if it has a valid offset (indicating push constant)
            if (offset != static_cast<size_t>(-1) && size > 0)
            {
                total_size += size;
                if (range_offset == -1)
                    range_offset = offset;

                LOGGER_TRACE("Push Constant Range:");
                LOGGER_TRACE("  Name: {}", param_name);
                LOGGER_TRACE("  Size: {}", size);
                LOGGER_TRACE("  Offset: {}", offset);
                LOGGER_TRACE("-------------------");
            }
        }
    }

    if (total_size > 0)
    {
        shader_reflection::PushConstantsRange push_range;
        push_range.stage = current_stage;
        push_range.offset = range_offset;
        push_range.size = total_size;
        reflection.push_constants.push_back(push_range);
    }
}

void ShaderCompiler::process_parameter_block_parameter(
    ShaderReflection& reflection,
    const char* name,
    slang::TypeLayoutReflection* type_layout,
    const int space,
    size_t binding_index,
    slang::ProgramLayout* program_layout
)
{
    const auto element_type_layout = type_layout->getElementTypeLayout();
    const auto descriptor_set = static_cast<size_t>(space);
    const StringId name_id = STRING_ID(name);

    // Check if this parameter block contains resources and uniform data
    const auto field_count = element_type_layout->getFieldCount();
    bool has_uniform_data = false;
    size_t resource_binding_counter = 1; // Resources in parameter blocks start from binding 1, 0 is for uniform buffer

    // First pass: process resources within the parameter block
    for (unsigned int i = 0; i < field_count; ++i)
    {
        const auto field = element_type_layout->getFieldByIndex(i);
        if (!field) continue;

        const auto field_name = field->getName();
        if (!field_name) continue;

        const auto field_layout = field->getTypeLayout();
        const auto field_type = field_layout->getType();

        if (field_type && field_type->getKind() == slang::TypeReflection::Kind::Struct && std::strcmp(field_type->getName(), "Conditional") == 0)
        {
            auto conditional_field_count = field_layout->getFieldCount();
            if (conditional_field_count != 1)
            {
                LOGGER_ERROR("Conditional field count is not 1, skipping");
                continue;
            }

            // Use the specialization-aware getElementCount overload to determine
            // if the Conditional's storage array is active (size 1) or inactive (size 0).
            // The zero-argument overload doesn't account for specialization and always returns 0.
            auto storage = field_layout->getFieldByIndex(0);
            auto element_count = storage->getTypeLayout()->getElementCount(program_layout);
            if (element_count == 0)
            {
                resource_binding_counter++;
                continue;
            }

            auto resource_type = storage->getTypeLayout()->getElementTypeLayout();
            // TODO: support conditionals that are not textures

            const auto resource_shape = resource_type->getResourceShape();
            const auto base_shape = resource_shape & SLANG_RESOURCE_BASE_SHAPE_MASK;
            const auto resource_name_id = STRING_ID(fmt::format("{}.{}", name, field_name));

            if ((resource_shape & SLANG_TEXTURE_COMBINED_FLAG) != 0)
                add_image_descriptor(
                    reflection,
                    resource_name_id,
                    DescriptorType::CombinedImageSampler,
                    ShaderStage::All,
                    descriptor_set,
                    resource_binding_counter,
                    base_shape,
                    storage->getTypeLayout()
                );
            else
                add_image_descriptor(
                    reflection,
                    resource_name_id,
                    DescriptorType::SampledImage,
                    ShaderStage::All,
                    descriptor_set,
                    resource_binding_counter,
                    base_shape,
                    field_layout
                );

            resource_binding_counter++;
        }
        else if (field_type && field_type->getKind() == slang::TypeReflection::Kind::Resource)
        {
            const auto resource_shape = field_type->getResourceShape();
            const auto base_shape = resource_shape & SLANG_RESOURCE_BASE_SHAPE_MASK;
            const auto resource_name_id = STRING_ID(fmt::format("{}.{}", name, field_name));

            const auto type = (resource_shape & SLANG_TEXTURE_COMBINED_FLAG) != 0
                                  ? DescriptorType::CombinedImageSampler
                                  : DescriptorType::SampledImage;

            add_image_descriptor(
                reflection,
                resource_name_id,
                type,
                ShaderStage::All,
                descriptor_set,
                resource_binding_counter,
                base_shape,
                field_layout
            );
            resource_binding_counter++;
        }
        else
        {
            // This is uniform data
            has_uniform_data = true;
        }
    }

    // Second pass: process uniform buffer if it contains uniform data
    if (has_uniform_data)
    {
        add_buffer_descriptor(
            reflection,
            name_id,
            DescriptorType::UniformBuffer,
            ShaderStage::All,
            descriptor_set,
            binding_index,
            element_type_layout
        );
    }

    LOGGER_TRACE("-------------------");
}

void ShaderCompiler::process_push_constant_parameter(
    ShaderReflection& reflection,
    const char* name,
    slang::TypeLayoutReflection* type_layout,
    size_t offset
) const
{
    const auto size = type_layout->getSize();

    shader_reflection::PushConstantsRange push_range{};
    push_range.stage = current_stage;
    push_range.offset = offset;
    push_range.size = size;

    reflection.push_constants.push_back(push_range);

    LOGGER_TRACE("Push Constant Range:");
    LOGGER_TRACE("  Name: {}", name);
    LOGGER_TRACE("  Size: {}", size);
    LOGGER_TRACE("  Offset: {}", offset);
    LOGGER_TRACE("-------------------");
}

void ShaderCompiler::process_buffer_uniforms(
    shader_reflection::BufferDescriptor& buffer,
    slang::TypeLayoutReflection* type_layout,
    StringId buffer_name,
    const size_t buffer_offset
)
{
    const auto field_count = type_layout->getFieldCount();
    for (unsigned int i = 0; i < field_count; ++i)
    {
        const auto field = type_layout->getFieldByIndex(i);
        const auto field_layout = field->getTypeLayout();
        const auto member_name = field->getName();

        if (!member_name) continue;

        // Check if this field is a resource (texture, sampler, etc.)
        const auto field_type = field_layout->getType();
        if (field_type && field_type->getKind() == slang::TypeReflection::Kind::Resource)
        {
            // Skip resources - they should be handled as separate descriptors, not uniform buffer fields
            continue;
        }

        // If this field is a struct, recurse into it to produce leaf-level uniform entries
        if (field_type && field_type->getKind() == slang::TypeReflection::Kind::Struct)
        {
            const auto nested_name = STRING_ID(fmt::format("{}.{}", buffer_name.string, member_name));
            process_buffer_uniforms(buffer, field_layout, nested_name, buffer_offset);
            continue;
        }

        // If this field is an array of structs, reflect the struct type and emit a single array uniform
        if (field_type && field_type->getKind() == slang::TypeReflection::Kind::Array)
        {
            const auto element_layout = field_layout->getElementTypeLayout();
            const auto element_type = element_layout ? element_layout->getType() : nullptr;
            if (element_type && element_type->getKind() == slang::TypeReflection::Kind::Struct)
            {
                const auto struct_name = STRING_ID(element_type->getName());

                // Reflect the struct type if not already registered
                if (!buffer.struct_types.contains(struct_name))
                {
                    buffer.struct_types[struct_name] = reflect_struct_type(element_layout, struct_name);
                }

                auto field_type_name = element_type->getName();

                // Emit a single uniform for the whole array
                const auto uniform_name = STRING_ID(fmt::format("{}.{}", buffer_name.string, member_name));
                shader_reflection::Uniform uniform;
                uniform.name = uniform_name;
                uniform.size = field_layout->getSize();
                uniform.offset = field->getOffset() - buffer_offset;
                uniform.property = {
                    .value = Buffer::copy(field_type_name, std::strlen(field_type_name)),
                    .type = reflection::PropertyType::object,
                    .container_type = reflection::PropertyContainerType::array,
                    .elements_number = static_cast<size_t>(field_layout->getElementCount())
                };
                buffer.uniforms[uniform_name] = std::move(uniform);
                continue;
            }
        }

        const auto uniform_name = STRING_ID(fmt::format("{}.{}", buffer_name.string, member_name));
        const auto size = field_layout->getSize();
        const auto offset = field->getOffset() - buffer_offset;
        auto field_type_name = field_layout->getName();

        shader_reflection::Uniform uniform;
        uniform.name = uniform_name;
        uniform.size = size;
        uniform.offset = offset;
        uniform.property = {
            .value = Buffer::copy(field_type_name, std::strlen(field_type_name)),
            .type = to_property_type(field_layout),
            .container_type = to_property_container_type(field_layout),
            .elements_number = get_element_number(field_layout)
        };

        buffer.uniforms[uniform_name] = std::move(uniform);
    }
}

size_t ShaderCompiler::get_image_dimensions_from_shape(unsigned int base_shape)
{
    switch (base_shape)
    {
    case SLANG_TEXTURE_1D:
        return 1;
    case SLANG_TEXTURE_2D:
        return 2;
    case SLANG_TEXTURE_3D:
        return 3;
    case SLANG_TEXTURE_CUBE:
        return 2; // Cube maps are 2D faces
    default:
        return 2; // Default to 2D
    }
}

size_t ShaderCompiler::get_array_size(slang::TypeLayoutReflection* type_layout)
{
    auto type_reflection = type_layout->getType();
    if (!type_reflection) return 1;

    // Check if this is an array type
    if (type_reflection->getKind() == slang::TypeReflection::Kind::Array)
    {
        const auto array_size = type_reflection->getElementCount();
        // If array size is 0, it means unbounded array, set to 1 as default
        return array_size > 0 ? static_cast<size_t>(array_size) : 1;
    }

    return 1; // Not an array, single element
}

shader_reflection::ReflectedStruct ShaderCompiler::reflect_struct_type(
    slang::TypeLayoutReflection* struct_layout,
    StringId struct_name
)
{
    shader_reflection::ReflectedStruct result;
    result.name = struct_name;
    result.stride = struct_layout->getSize();

    LOGGER_TRACE("Struct Declaration:");
    LOGGER_TRACE("  {}", struct_name.string);
    LOGGER_TRACE("  Size: {}, Stride: {}", struct_layout->getSize(), struct_layout->getStride());

    const auto field_count = struct_layout->getFieldCount();
    LOGGER_TRACE("  Fields:");
    for (unsigned int i = 0; i < field_count; ++i)
    {
        const auto field = struct_layout->getFieldByIndex(i);
        const auto field_layout = field->getTypeLayout();

        shader_reflection::StructField sf;
        sf.name = STRING_ID(field->getName());
        sf.size = field_layout->getSize();
        sf.offset = field->getOffset();
        sf.property = {
            .value = Buffer::copy(sf.name.string.data(), sf.name.string.size()),
            .type = to_property_type(field_layout),
            .container_type = to_property_container_type(field_layout),
            .elements_number = get_element_number(field_layout)
        };
        result.fields.push_back(sf);
        LOGGER_TRACE("    {}: {}", sf.name.string, format_property(sf.property));
    }
    LOGGER_TRACE("-------------------");

    return result;
}
} // portal
