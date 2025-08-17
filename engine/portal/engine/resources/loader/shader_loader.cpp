//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "shader_loader.h"

#include <numeric>

#include "slang.h"
#include "slang-com-ptr.h"
#include "portal/core/files/file_system.h"

#include "portal/engine/renderer/descriptor_layout_builder.h"
#include "portal/engine/resources/resource_registry.h"
#include "portal/engine/resources/source/resource_source.h"

namespace portal::resources
{

static auto logger = Log::get_logger("Resources");

ShaderLoader::ShaderLoader(ResourceRegistry* registry, const std::shared_ptr<GpuContext>& context) : ResourceLoader(registry), context(context)
{
    slang::createGlobalSession(slang_session.writeRef());
}

bool ShaderLoader::load(const std::shared_ptr<ResourceSource> source) const
{
    auto shader = registry->get<Shader>(source->get_meta().source_id);
    auto meta = source->get_meta();

    if (meta.format == SourceFormat::Shader)
        return load_shader(source, shader);
    if (meta.format == SourceFormat::PrecompiledShader)
        return load_precompiled_shader(source, shader);

    LOGGER_ERROR("Unknown shader format: {}", meta.format);
    return false;
}

void ShaderLoader::load_default(Ref<Resource>& resource) const
{
    LOGGER_WARN("No default shader loader for resource: {}", resource->id);
}

bool ShaderLoader::load_precompiled_shader(const std::shared_ptr<ResourceSource>& source, Ref<Shader>& shader) const
{
    auto data = source->load();

    const auto builder = vulkan::DescriptorLayoutBuilder{}
                         .add_binding(0, vk::DescriptorType::eUniformBuffer)
                         .add_binding(1, vk::DescriptorType::eCombinedImageSampler)
                         .add_binding(2, vk::DescriptorType::eCombinedImageSampler);
    shader->descriptor_layout = std::make_shared<vk::raii::DescriptorSetLayout>(context->create_descriptor_set_layout(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, builder));
    shader->code = Buffer::copy(data);

    shader->shader_data[vk::ShaderStageFlagBits::eVertex] = {
        .push_constant_range = vk::PushConstantRange{
            .stageFlags = vk::ShaderStageFlagBits::eVertex,
            .offset = 0,
            .size = sizeof(vulkan::GPUDrawPushConstants)
        },
        .shader_module = std::make_shared<vk::raii::ShaderModule>(context->create_shader_module(shader->code)),
        .entry_point = "vert_main"
    };

    shader->shader_data[vk::ShaderStageFlagBits::eFragment] = {
        .push_constant_range = {},
        .shader_module = std::make_shared<vk::raii::ShaderModule>(context->create_shader_module(shader->code)),
        .entry_point = "frag_main"
    };

    return true;
}

bool ShaderLoader::load_shader(const std::shared_ptr<ResourceSource>& source, Ref<Shader>& shader) const
{
    compile_shaders(source, shader);
    return true;
}

void diagnose_if_needed(const Slang::ComPtr<slang::IBlob>& diagnostics_blob)
{
    if (diagnostics_blob != nullptr)
    {
        const auto diagnostics = static_cast<const char*>(diagnostics_blob->getBufferPointer());
        LOGGER_TRACE("slang diagnostics: {}", diagnostics);
    }
}

void ShaderLoader::compile_shaders(const std::shared_ptr<ResourceSource>& source, Ref<Shader>& shader) const
{
    auto meta = source->get_meta();
    auto data = source->load();
    auto null_terminated_data = Buffer::copy(data.data, data.size + 1);
    null_terminated_data[data.size] = '\0';

    slang::TargetDesc target_desc{
        .format = SLANG_SPIRV,
        .profile = slang_session->findProfile("spirv_1_4")
    };

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
            slang::CompilerOptionName::MatrixLayoutColumn,  // Try forcing column-major layout
            slang::CompilerOptionValue{slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
        },
        {
            slang::CompilerOptionName::VulkanBindShift,  // Ensure proper binding behavior
            slang::CompilerOptionValue{slang::CompilerOptionValueKind::Int, 0, 0, nullptr, nullptr}
        }
    }};

    const slang::SessionDesc session_desc{
        .targets = &target_desc,
        .targetCount = 1,
        .compilerOptionEntries = options.data(),
        .compilerOptionEntryCount = static_cast<uint32_t>(options.size())
    };

    Slang::ComPtr<slang::ISession> session;
    slang_session->createSession(session_desc, session.writeRef());

    Slang::ComPtr<slang::IModule> module;
    {
        Slang::ComPtr<slang::IBlob> diagnostics_blob;
        module = session->loadModuleFromSourceString(
            meta.source_id.string.data(),
            nullptr,
            null_terminated_data.as<const char*>(),
            diagnostics_blob.writeRef()
            );
        diagnose_if_needed(diagnostics_blob);
        if (!module)
        {
            LOGGER_ERROR("Failed to load shader: {}", meta.source_id);
            return;
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
                continue;
        }

        auto entry_point_name = entry_point->getFunctionReflection()->getName();
        LOGGER_TRACE("found entry point: {}", entry_point_name);

        component_types.push_back(entry_point);
    }

    Slang::ComPtr<slang::IComponentType> composed_program;
    {
        Slang::ComPtr<slang::IBlob> diagnostics_blob;
        const auto result = session->createCompositeComponentType(
            component_types.data(),
            component_types.size(),
            composed_program.writeRef(),
            diagnostics_blob.writeRef()
            );
        diagnose_if_needed(diagnostics_blob);

        if (SLANG_FAILED(result))
        {
            LOGGER_ERROR("Failed to create composite component type");
            return;
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
            return;
        }
    }

    Slang::ComPtr<slang::IBlob> spirv_code;
    {
        Slang::ComPtr<slang::IBlob> diagnostics_blob;
        const auto result = linked_program->getTargetCode(0, spirv_code.writeRef(), diagnostics_blob.writeRef());
        diagnose_if_needed(diagnostics_blob);

        if (SLANG_FAILED(result))
        {
            LOGGER_ERROR("Failed to compile program");
            return;
        }
    }

    shader->code = Buffer::copy(spirv_code->getBufferPointer(), spirv_code->getBufferSize());

    auto layout = linked_program->getLayout();
    reflect_shader(layout, shader);

}

vk::DescriptorType to_descriptor_type(slang::TypeLayoutReflection* layout)
{
    const auto kind = layout->getKind();
    switch (kind)
    {
    case slang::TypeReflection::Kind::ParameterBlock:
        return vk::DescriptorType::eUniformBuffer;
    case slang::TypeReflection::Kind::SamplerState:
        return vk::DescriptorType::eSampler;
    case slang::TypeReflection::Kind::ConstantBuffer:
    {
        // Check if this is a dynamic buffer by examining the variable layout
        // Dynamic buffers are typically identified by:
        // 1. Being in a parameter block that's meant to be updated frequently
        // 2. Having certain usage patterns or annotations
        // 3. Being part of a buffer that will use dynamic offsets

        // Method 1: Check for dynamic usage through variable reflection
        const auto var_layout = layout->getElementVarLayout();
        if (var_layout)
        {
            // Check if this buffer is marked for dynamic usage
            const auto category = var_layout->getCategory();
            if (category == slang::ParameterCategory::Uniform)
            {
                const auto type = var_layout->getTypeLayout();
                if (type && type->getSize() > 0)
                {
                    // Heuristic: Large uniform buffers are often dynamic
                    // You might want to adjust this threshold based on your needs
                    constexpr size_t DYNAMIC_BUFFER_THRESHOLD = 256; // bytes
                    if (type->getSize() > DYNAMIC_BUFFER_THRESHOLD)
                        return vk::DescriptorType::eUniformBufferDynamic;
                }
            }
        }
        return vk::DescriptorType::eUniformBuffer;
    }
    case slang::TypeReflection::Kind::Resource:
    {
        auto shape = layout->getResourceShape();
        auto access = layout->getResourceAccess();

        switch (shape & SLANG_RESOURCE_BASE_SHAPE_MASK)
        {
        case SLANG_TEXTURE_1D:
        case SLANG_TEXTURE_2D:
        case SLANG_TEXTURE_3D:
        case SLANG_TEXTURE_CUBE:
        {
            // Check for combined image sampler first
            if (shape & SLANG_TEXTURE_COMBINED_FLAG)
                return vk::DescriptorType::eCombinedImageSampler;

            // Check access pattern for storage vs sampled
            if (access == SLANG_RESOURCE_ACCESS_READ_WRITE ||
                access == SLANG_RESOURCE_ACCESS_RASTER_ORDERED ||
                access == SLANG_RESOURCE_ACCESS_WRITE)
                return vk::DescriptorType::eStorageImage;

            // Default to sampled image for read-only textures
            return vk::DescriptorType::eSampledImage;
        }
        case SLANG_TEXTURE_BUFFER:
        {
            // Distinguish between uniform and storage texel buffers based on access
            if (access == SLANG_RESOURCE_ACCESS_READ_WRITE ||
                access == SLANG_RESOURCE_ACCESS_WRITE)
                return vk::DescriptorType::eStorageTexelBuffer;
            return vk::DescriptorType::eUniformTexelBuffer;
        }
        case SLANG_STRUCTURED_BUFFER:
        case SLANG_BYTE_ADDRESS_BUFFER:
        {
            // Similar dynamic detection logic for storage buffers
            const char* buffer_name = layout->getName();
            if (buffer_name && strstr(buffer_name, "dynamic"))
                return vk::DescriptorType::eStorageBufferDynamic;

            return vk::DescriptorType::eStorageBuffer;
        }
        case SLANG_ACCELERATION_STRUCTURE:
            return vk::DescriptorType::eAccelerationStructureKHR;

        default:
            LOGGER_WARN("Unknown resource shape");
        }
        break;
    }
    default:
        break;
    }

    LOGGER_WARN("Unknown type kind");
    return vk::DescriptorType::eUniformBuffer;
}

vk::ShaderStageFlagBits to_shader_stage(const SlangStage stage)
{
    switch (stage)
    {
    case SLANG_STAGE_NONE:
        return vk::ShaderStageFlagBits::eAll;
    case SLANG_STAGE_VERTEX:
        return vk::ShaderStageFlagBits::eVertex;
    case SLANG_STAGE_HULL:
        return vk::ShaderStageFlagBits::eTessellationControl;
    case SLANG_STAGE_DOMAIN:
        return vk::ShaderStageFlagBits::eTessellationEvaluation;
    case SLANG_STAGE_GEOMETRY:
        return vk::ShaderStageFlagBits::eGeometry;
    case SLANG_STAGE_FRAGMENT:
        return vk::ShaderStageFlagBits::eFragment;
    case SLANG_STAGE_COMPUTE:
        return vk::ShaderStageFlagBits::eCompute;
    case SLANG_STAGE_RAY_GENERATION:
        return vk::ShaderStageFlagBits::eRaygenKHR;
    case SLANG_STAGE_INTERSECTION:
        return vk::ShaderStageFlagBits::eIntersectionKHR;
    case SLANG_STAGE_ANY_HIT:
        return vk::ShaderStageFlagBits::eAnyHitKHR;
    case SLANG_STAGE_CLOSEST_HIT:
        return vk::ShaderStageFlagBits::eClosestHitKHR;
    case SLANG_STAGE_MISS:
        return vk::ShaderStageFlagBits::eMissKHR;
    case SLANG_STAGE_CALLABLE:
        return vk::ShaderStageFlagBits::eCallableKHR;
    case SLANG_STAGE_MESH:
        return vk::ShaderStageFlagBits::eMeshEXT;
    default:
        return vk::ShaderStageFlagBits::eAll;
    }
}

void populate_descriptors(vulkan::DescriptorLayoutBuilder& builder, slang::VariableLayoutReflection* scope)
{
    const auto scope_type = scope->getTypeLayout();
    const auto kind = scope_type->getKind();
    switch (kind)
    {
    case slang::TypeReflection::Kind::Struct:
    {
        for (unsigned int i = 0; i < scope_type->getFieldCount(); i++)
        {
            auto* field = scope_type->getFieldByIndex(i);
            const auto space = field->getBindingSpace();
            const auto binding = field->getBindingIndex();
            auto* type_layout = field->getTypeLayout();

            LOGGER_TRACE("Field {}: space={}, binding={}, name={}", i, space, binding, field->getName() ? field->getName() : "unnamed");

            // space 0 is global descriptors
            if (space != 0)
            {
                auto desc_type = to_descriptor_type(type_layout);
                builder.add_binding(binding, desc_type);
                LOGGER_TRACE("Added binding {} with type {}", binding, vk::to_string(desc_type));
            }
        }
        break;
    }
    case slang::TypeReflection::Kind::ConstantBuffer:
    {
        populate_descriptors(builder, scope_type->getElementVarLayout());
        break;
    }
    case slang::TypeReflection::Kind::ParameterBlock:
        populate_descriptors(builder, scope_type->getElementVarLayout());
        break;
    default:
        LOGGER_WARN("Unknown type kind");
        break;
    }
}

void ShaderLoader::reflect_shader(slang::ProgramLayout* layout, Ref<Shader>& shader) const
{
    const auto entry_points = layout->getEntryPointCount();
    for (unsigned int i = 0; i < entry_points; i++)
    {
        const auto entry = layout->getEntryPointByIndex(i);
        auto stage = to_shader_stage(entry->getStage());

        const auto entry_layout = entry->getVarLayout();
        const auto push_constant_fields = entry_layout->getTypeLayout()->getElementVarLayout()->getTypeLayout();

        uint32_t total_size = 0;
        for (unsigned int f = 0; f < push_constant_fields->getFieldCount(); f++)
        {
            auto* field = push_constant_fields->getFieldByIndex(f);
            auto* type_layout = field->getTypeLayout();
            const uint32_t size = static_cast<uint32_t>(type_layout->getSize());
            total_size += size;
        }

        if (total_size != 0)
        {
            shader->shader_data[stage].push_constant_range = {
                .stageFlags = stage,
                .offset = 0,
                .size = total_size
            };
        }
        shader->shader_data[stage].entry_point = entry->getName();
        shader->shader_data[stage].shader_module = std::make_shared<vk::raii::ShaderModule>(context->create_shader_module(shader->code));
    }

    auto global_descriptors_builder = vulkan::DescriptorLayoutBuilder{};
    populate_descriptors(global_descriptors_builder, layout->getGlobalParamsVarLayout());

    const auto combined_stages = std::accumulate(
        shader->shader_data.begin(),
        shader->shader_data.end(),
        vk::ShaderStageFlags{},
        [](vk::ShaderStageFlags flags, const auto& pair) { return flags | pair.first; }
        );

    shader->descriptor_layout = std::make_shared<vk::raii::DescriptorSetLayout>(context->create_descriptor_set_layout(combined_stages, global_descriptors_builder));

    Slang::ComPtr<slang::IBlob> json_blob;
    layout->toJson(json_blob.writeRef());
    Buffer json_data = Buffer::copy(json_blob->getBufferPointer(), json_blob->getBufferSize());
    FileSystem::write_file(fmt::format("{}.spv", shader->id.string), std::string(shader->code.as<const char*>(), shader->code.size));
    FileSystem::write_file(fmt::format("{}.json", shader->id.string), std::string(json_data.as<const char*>(), json_data.size));
}
} // portal
