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
        LOGGER_TRACE("slang diagnostics: {}", diagnostics);
    }
}

std::string format_property(reflection::Property prop)
{
    switch (prop.container_type)
    {
    case reflection::PropertyContainerType::scalar:
        return std::string(to_string(prop.type));
    case reflection::PropertyContainerType::array:
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

size_t ShaderCompiler::get_image_dimensions(slang::TypeLayoutReflection* type_layout)
{
    // Get the base type for texture/image resources
    auto type_reflection = type_layout->getType();
    if (!type_reflection) return 2; // Default to 2D

    // Check if this is a resource type (texture, image, etc.)
    if (type_reflection->getKind() == slang::TypeReflection::Kind::Resource)
    {
        // Get the resource shape which contains dimension information
        auto resource_shape = type_reflection->getResourceShape();
        auto base_shape = resource_shape & SLANG_RESOURCE_BASE_SHAPE_MASK;

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

    // For array types, get the element type and check its dimensions
    if (type_reflection->getKind() == slang::TypeReflection::Kind::Array)
    {
        auto element_type = type_reflection->getElementType();
        if (element_type && element_type->getKind() == slang::TypeReflection::Kind::Resource)
        {
            auto resource_shape = element_type->getResourceShape();
            auto base_shape = resource_shape & SLANG_RESOURCE_BASE_SHAPE_MASK;

            switch (base_shape)
            {
            case SLANG_TEXTURE_1D:
                return 1;
            case SLANG_TEXTURE_2D:
                return 2;
            case SLANG_TEXTURE_3D:
                return 3;
            case SLANG_TEXTURE_CUBE:
                return 2;
            default:
                return 2;
            }
        }
    }

    return 2; // Default to 2D texture
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
    LOGGER_DEBUG("Found {} entry points in shader", entry_point_count);

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
    LOGGER_TRACE("===========================");
    LOGGER_TRACE(" Slang Shader Reflection");
    LOGGER_TRACE("===========================");

    ShaderReflection reflection;

    process_variable_layout(reflection, layout->getGlobalParamsVarLayout());

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

void ShaderCompiler::process_variable_layout(ShaderReflection& reflection, slang::VariableLayoutReflection* var_layout)
{
    if (!var_layout) return;

    // Process parameters directly from variable layout
    process_parameters_from_variable_layout(reflection, var_layout);
}

void ShaderCompiler::process_parameters_from_variable_layout(ShaderReflection& reflection, slang::VariableLayoutReflection* var_layout)
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
            process_constant_buffer_parameter(reflection, field_name, field_type_layout, space, binding_index);
            break;

        case slang::TypeReflection::Kind::Resource:
            {
                has_global_variables = true;
                // Check if it's a combined texture sampler
                const auto resource_shape = field_type->getResourceShape();
                const auto base_shape = resource_shape & SLANG_RESOURCE_BASE_SHAPE_MASK;

                if ((resource_shape & SLANG_TEXTURE_COMBINED_FLAG) != 0)
                {
                    process_combined_texture_sampler_parameter(reflection, field_name, field_type_layout, space, binding_index, base_shape);
                }
                else
                {
                    // Handle separate textures, samplers, etc.
                    process_resource_parameter(reflection, field_name, field_type_layout, space, binding_index, resource_shape);
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
                process_parameter_block_parameter(reflection, field_name, field_type_layout, parameter_block_space_counter, 0);
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


void ShaderCompiler::process_buffer_uniforms(
    shader_reflection::BufferDescriptor& buffer,
    slang::TypeLayoutReflection* type_layout,
    StringId buffer_name,
    size_t buffer_offset
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

        const auto uniform_name = STRING_ID(fmt::format("{}.{}", buffer_name.string, member_name));
        const auto size = field_layout->getSize();
        const auto offset = field->getOffset() - buffer_offset;

        shader_reflection::Uniform uniform;
        uniform.name = uniform_name;
        uniform.size = size;
        uniform.offset = offset;
        uniform.property = {
            .type = to_property_type(field_layout),
            .container_type = to_property_container_type(field_layout),
            .elements_number = get_element_number(field_layout)
        };

        buffer.uniforms[uniform_name] = uniform;
    }
}


size_t ShaderCompiler::get_array_size(slang::TypeLayoutReflection* type_layout)
{
    auto type_reflection = type_layout->getType();
    if (!type_reflection) return 1;

    // Check if this is an array type
    if (type_reflection->getKind() == slang::TypeReflection::Kind::Array)
    {
        auto array_size = type_reflection->getElementCount();
        // If array size is 0, it means unbounded array, set to 1 as default
        return array_size > 0 ? static_cast<size_t>(array_size) : 1;
    }

    return 1; // Not an array, single element
}

void ShaderCompiler::process_constant_buffer_parameter(
    ShaderReflection& reflection,
    const char* name,
    slang::TypeLayoutReflection* type_layout,
    int space,
    size_t binding_index
)
{
    const auto buffer_size = type_layout->getElementTypeLayout()->getSize();
    const auto descriptor_set = static_cast<size_t>(space);
    const StringId name_id = STRING_ID(name);

    // Ensure descriptor set exists
    if (descriptor_set >= reflection.descriptor_sets.size())
        reflection.descriptor_sets.resize(descriptor_set + 1);

    auto& desc_set = reflection.descriptor_sets[descriptor_set];
    shader_reflection::BufferDescriptor uniform_buffer;
    uniform_buffer.type = DescriptorType::UniformBuffer;
    uniform_buffer.stage = ShaderStage::All;
    uniform_buffer.binding_point = binding_index;
    uniform_buffer.name = name_id;
    uniform_buffer.size = buffer_size;
    uniform_buffer.offset = 0;
    uniform_buffer.range = buffer_size;
    process_buffer_uniforms(uniform_buffer, type_layout->getElementTypeLayout(), name_id, 0);

    LOGGER_TRACE("Uniform Buffers:");
    LOGGER_TRACE("  {} ({}, {})", name, descriptor_set, binding_index);
    LOGGER_TRACE("  Size: {}", buffer_size);
    LOGGER_TRACE("  Fields:");
    for (const auto& [fst, snd] : uniform_buffer.uniforms)
        LOGGER_TRACE("    {}: {}", fst.string, format_property(snd.property));
    LOGGER_TRACE("-------------------");

    desc_set.uniform_buffers[binding_index] = uniform_buffer;

    reflection.resources[name_id] = shader_reflection::ShaderResourceDeclaration{
        .name = name_id,
        .type = DescriptorType::UniformBuffer,
        .set = descriptor_set,
        .binding_index = binding_index,
        .count = 1
    };
}

void ShaderCompiler::process_parameter_block_parameter(
    ShaderReflection& reflection,
    const char* name,
    slang::TypeLayoutReflection* type_layout,
    int space,
    size_t binding_index
)
{
    const auto element_type_layout = type_layout->getElementTypeLayout();
    const auto descriptor_set = static_cast<size_t>(space);
    const StringId name_id = STRING_ID(name);

    // Ensure descriptor set exists
    if (descriptor_set >= reflection.descriptor_sets.size())
        reflection.descriptor_sets.resize(descriptor_set + 1);

    auto& desc_set = reflection.descriptor_sets[descriptor_set];

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

        if (field_type && field_type->getKind() == slang::TypeReflection::Kind::Resource)
        {
            // This is a resource within the parameter block
            const auto resource_shape = field_type->getResourceShape();
            const auto base_shape = resource_shape & SLANG_RESOURCE_BASE_SHAPE_MASK;

            if ((resource_shape & SLANG_TEXTURE_COMBINED_FLAG) != 0)
            {
                // Combined texture sampler
                const auto resource_name_id = STRING_ID(fmt::format("{}.{}", name, field_name));

                shader_reflection::ImageSamplerDescriptor image_sampler;
                image_sampler.type = DescriptorType::CombinedImageSampler;
                image_sampler.stage = ShaderStage::All;
                image_sampler.binding_point = resource_binding_counter;
                image_sampler.descriptor_set = descriptor_set;
                image_sampler.name = resource_name_id;
                image_sampler.dimensions = get_image_dimensions_from_shape(base_shape);
                image_sampler.array_size = get_array_size(field_layout);

                desc_set.image_samplers[resource_binding_counter] = image_sampler;

                reflection.resources[resource_name_id] = shader_reflection::ShaderResourceDeclaration{
                    .name = resource_name_id,
                    .type = DescriptorType::CombinedImageSampler,
                    .set = descriptor_set,
                    .binding_index = resource_binding_counter,
                    .count = get_array_size(field_layout)
                };

                LOGGER_TRACE("Parameter Block Resource - Combined Image Sampler:");
                LOGGER_TRACE("  {} ({}, {})", resource_name_id.string, descriptor_set, resource_binding_counter);
                LOGGER_TRACE("  Dimensions: {}D", image_sampler.dimensions);
                LOGGER_TRACE("  Array Size: {}", image_sampler.array_size);

                resource_binding_counter++;
            }
            else
            {
                // Separate texture/sampler
                const auto resource_name_id = STRING_ID(fmt::format("{}.{}", name, field_name));

                shader_reflection::ImageSamplerDescriptor image;
                image.type = DescriptorType::SampledImage;
                image.stage = ShaderStage::All;
                image.binding_point = resource_binding_counter;
                image.descriptor_set = descriptor_set;
                image.name = resource_name_id;
                image.dimensions = get_image_dimensions_from_shape(base_shape);
                image.array_size = get_array_size(field_layout);

                desc_set.images[resource_binding_counter] = image;

                reflection.resources[resource_name_id] = shader_reflection::ShaderResourceDeclaration{
                    .name = resource_name_id,
                    .type = DescriptorType::SampledImage,
                    .set = descriptor_set,
                    .binding_index = resource_binding_counter,
                    .count = get_array_size(field_layout)
                };

                LOGGER_TRACE("Parameter Block Resource - Separate Image:");
                LOGGER_TRACE("  {} ({}, {})", resource_name_id.string, descriptor_set, resource_binding_counter);
                LOGGER_TRACE("  Dimensions: {}D", image.dimensions);
                LOGGER_TRACE("  Array Size: {}", image.array_size);

                resource_binding_counter++;
            }
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
        const auto buffer_size = element_type_layout->getSize();

        shader_reflection::BufferDescriptor uniform_buffer;
        uniform_buffer.type = DescriptorType::UniformBuffer;
        uniform_buffer.stage = ShaderStage::All;
        uniform_buffer.binding_point = binding_index;
        uniform_buffer.name = name_id;
        uniform_buffer.size = buffer_size;
        uniform_buffer.offset = 0;
        uniform_buffer.range = buffer_size;
        process_buffer_uniforms(uniform_buffer, element_type_layout, name_id, 0);

        LOGGER_TRACE("Parameter Block - Uniform Buffer:");
        LOGGER_TRACE("  {} ({}, {})", name, descriptor_set, binding_index);
        LOGGER_TRACE("  Size: {}", buffer_size);
        LOGGER_TRACE("  Fields:");
        for (const auto& [fst, snd] : uniform_buffer.uniforms)
            LOGGER_TRACE("    {}: {}", fst.string, format_property(snd.property));

        desc_set.uniform_buffers[binding_index] = uniform_buffer;

        reflection.resources[name_id] = shader_reflection::ShaderResourceDeclaration{
            .name = name_id,
            .type = DescriptorType::UniformBuffer,
            .set = descriptor_set,
            .binding_index = binding_index,
            .count = 1
        };
    }

    LOGGER_TRACE("-------------------");
}

void ShaderCompiler::process_combined_texture_sampler_parameter(
    ShaderReflection& reflection,
    const char* name,
    slang::TypeLayoutReflection* type_layout,
    int space,
    size_t binding_index,
    unsigned int base_shape
) const
{
    const auto descriptor_set = static_cast<size_t>(space);
    const auto name_id = STRING_ID(name);

    // Ensure descriptor set exists
    if (descriptor_set >= reflection.descriptor_sets.size())
        reflection.descriptor_sets.resize(descriptor_set + 1);

    auto& desc_set = reflection.descriptor_sets[descriptor_set];

    shader_reflection::ImageSamplerDescriptor image_sampler;
    image_sampler.type = DescriptorType::CombinedImageSampler;
    image_sampler.stage = current_stage;
    image_sampler.binding_point = binding_index;
    image_sampler.descriptor_set = descriptor_set;
    image_sampler.name = name_id;
    image_sampler.dimensions = get_image_dimensions_from_shape(base_shape);
    image_sampler.array_size = get_array_size(type_layout);

    desc_set.image_samplers[binding_index] = image_sampler;

    LOGGER_TRACE("Combined Image Samplers:");
    LOGGER_TRACE("  {} ({}, {})", name, descriptor_set, binding_index);
    LOGGER_TRACE("  Dimensions: {}D", image_sampler.dimensions);
    LOGGER_TRACE("  Array Size: {}", image_sampler.array_size);
    LOGGER_TRACE("-------------------");

    reflection.resources[name_id] = shader_reflection::ShaderResourceDeclaration{
        .name = name_id,
        .type = DescriptorType::CombinedImageSampler,
        .set = descriptor_set,
        .binding_index = binding_index,
        .count = get_array_size(type_layout)
    };
}

void ShaderCompiler::process_resource_parameter(
    ShaderReflection& reflection,
    const char* name,
    slang::TypeLayoutReflection* type_layout,
    int space,
    size_t binding_index,
    unsigned int resource_shape
)
{
    // This handles separate textures, samplers, storage images, etc.
    const auto descriptor_set = static_cast<size_t>(space);
    const StringId name_id = STRING_ID(name);
    const auto base_shape = resource_shape & SLANG_RESOURCE_BASE_SHAPE_MASK;

    // Ensure descriptor set exists
    if (descriptor_set >= reflection.descriptor_sets.size())
        reflection.descriptor_sets.resize(descriptor_set + 1);

    auto& desc_set = reflection.descriptor_sets[descriptor_set];

    // For now, assume separate textures - can be extended later
    shader_reflection::ImageSamplerDescriptor image;
    image.type = DescriptorType::SampledImage;
    image.stage = current_stage;
    image.binding_point = binding_index;
    image.descriptor_set = descriptor_set;
    image.name = name_id;
    image.dimensions = get_image_dimensions_from_shape(base_shape);
    image.array_size = get_array_size(type_layout);

    desc_set.images[binding_index] = image;

    LOGGER_TRACE("Separate Images:");
    LOGGER_TRACE("  {} ({}, {})", name, descriptor_set, binding_index);
    LOGGER_TRACE("  Dimensions: {}D", image.dimensions);
    LOGGER_TRACE("  Array Size: {}", image.array_size);
    LOGGER_TRACE("-------------------");

    reflection.resources[name_id] = shader_reflection::ShaderResourceDeclaration{
        .name = name_id,
        .type = DescriptorType::SampledImage,
        .set = descriptor_set,
        .binding_index = binding_index,
        .count = get_array_size(type_layout)
    };
}

void ShaderCompiler::process_push_constant_parameter(
    ShaderReflection& reflection,
    const char* name,
    slang::TypeLayoutReflection* type_layout,
    size_t offset
)
{
    const auto size = type_layout->getSize();

    shader_reflection::PushConstantsRange push_range;
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
} // portal
