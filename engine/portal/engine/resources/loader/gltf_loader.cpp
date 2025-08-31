//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "gltf_loader.h"

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <glm/gtx/quaternion.hpp>

#include "portal/core/debug/profile.h"
#include "portal/engine/renderer/shaders/shader_cache.h"
#include "portal/engine/resources/resource_registry.h"
#include "portal/engine/resources/loader/loader_factory.h"
#include "portal/engine/resources/resources/material.h"
#include "portal/engine/resources/resources/mesh.h"
#include "portal/engine/renderer/vulkan/vulkan_pipeline.h"
#include "portal/engine/resources/resources/texture.h"
#include "portal/engine/resources/source/file_source.h"
#include "portal/engine/resources/source/memory_source.h"
#include "portal/engine/resources/source/resource_source.h"
#include "portal/engine/scene/scene.h"
#include "portal/engine/scene/nodes/mesh_node.h"
#include "portal/engine/scene/nodes/node.h"
#include "portal/engine/renderer/vulkan/gpu_context.h"


namespace portal::resources
{

static auto logger = Log::get_logger("Resources");
// const auto SHADER = "mesh.shading.slang.spv";
const auto SHADER = STRING_ID("pbr.slang");

vk::Filter extract_filter(const fastgltf::Filter filter)
{
    switch (filter)
    {
    case fastgltf::Filter::Nearest:
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::NearestMipMapLinear:
        return vk::Filter::eNearest;
    case fastgltf::Filter::Linear:
    case fastgltf::Filter::LinearMipMapLinear:
    case fastgltf::Filter::LinearMipMapNearest:
        return vk::Filter::eLinear;
    }
    return vk::Filter::eLinear;
}

vk::SamplerMipmapMode extract_mipmap_mode(const fastgltf::Filter filter)
{
    switch (filter)
    {
    case fastgltf::Filter::Nearest:
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::LinearMipMapNearest:
        return vk::SamplerMipmapMode::eNearest;
    case fastgltf::Filter::Linear:
    case fastgltf::Filter::NearestMipMapLinear:
    case fastgltf::Filter::LinearMipMapLinear:
        return vk::SamplerMipmapMode::eLinear;
    }
    return vk::SamplerMipmapMode::eLinear;
}

StringId create_name(ResourceType type, size_t index, std::string_view name)
{
    return STRING_ID(fmt::format("{}{}-{}", type, index, name));
}

GltfLoader::GltfLoader(ResourceRegistry* registry, const std::shared_ptr<renderer::vulkan::GpuContext>& context) : ResourceLoader(registry), gpu_context(context) {}

bool GltfLoader::load(const std::shared_ptr<ResourceSource> source) const
{
    PORTAL_PROF_ZONE;

    const auto data = source->load();
    auto data_result = fastgltf::GltfDataBuffer::FromBytes(data.as<std::byte*>(), data.size);
    if (!data_result)
    {
        LOGGER_ERROR("Failed to load glTF from: {}, error: {}", source->get_meta().source_id, fastgltf::getErrorMessage(data_result.error()));
        return false;
    }

    const auto gltf = load_from_source(source, data_result.get());

    LOGGER_TRACE("Loading gltf file with:");
    LOGGER_TRACE("  - {} nodes", gltf.nodes.size());
    LOGGER_TRACE("  - {} meshes", gltf.meshes.size());
    LOGGER_TRACE("  - {} materials", gltf.materials.size());
    LOGGER_TRACE("  - {} textures", gltf.textures.size());
    LOGGER_TRACE("  - {} images", gltf.images.size());
    LOGGER_TRACE("  - {} samplers", gltf.samplers.size());

    for (auto& texture : gltf.textures)
    {
        load_texture(gltf, texture);
    }

    int material_index = 0;
    for (auto& material : gltf.materials)
    {
        load_material(material_index++, gltf, material);
    }

    int mesh_index = 0;
    for (auto& mesh : gltf.meshes)
    {
        load_mesh(mesh_index++, gltf, mesh);
    }

    const auto scenes = load_scenes(gltf);

    if (scenes.empty())
        return false;

    auto& root_resource = registry->get(source->get_meta().source_id, ResourceType::Scene);
    root_resource = scenes.front();
    return true;
}

void GltfLoader::load_default(Ref<Resource>&) const
{
}

fastgltf::Asset GltfLoader::load_from_source(const std::shared_ptr<ResourceSource>& source, fastgltf::GltfDataGetter& data)
{
    auto meta = source->get_meta();
    const auto parent_path = meta.source_path.parent_path();

    constexpr auto glft_options = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble |
        fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages;

    fastgltf::Parser parser;

    auto load = parser.loadGltf(data, parent_path, glft_options);
    if (!load)
    {
        LOGGER_ERROR("Failed to load glTF {}, error: {}", meta.source_id, fastgltf::getErrorMessage(load.error()));
        return fastgltf::Asset{};
    }

    return std::move(load.get());
}

void GltfLoader::load_texture(
    const fastgltf::Asset& asset,
    const fastgltf::Texture& texture
    ) const
{
    std::shared_ptr<ResourceSource> image_source;
    auto image_index = texture.imageIndex;
    PORTAL_ASSERT(image_index.has_value(), "Texture references invalid image: {}", texture.name);
    auto& image = asset.images[image_index.value()];
    const size_t index = texture.imageIndex.value();
    auto texture_name = texture.name.empty() ? image.name : texture.name;

    auto texture_resource = registry->get<Texture>(create_name(ResourceType::Texture, index, texture_name.c_str()));
    if (texture_resource->is_valid())
        return;

    auto visitor = fastgltf::visitor{
        [&](std::monostate)
        {
            LOGGER_ERROR("Unsupported image source type 'monostate'");
        },
        [&](const fastgltf::sources::CustomBuffer&)
        {
            LOGGER_ERROR("Unsupported image source type 'CustomBuffer'");
        },
        [&](const fastgltf::sources::ByteView&)
        {
            LOGGER_ERROR("Unsupported image source type 'ByteView'");
        },
        [&](const fastgltf::sources::Fallback&)
        {
            LOGGER_ERROR("Unsupported image source type 'Fallback'");
        },
        [&](const fastgltf::sources::URI& uri_source)
        {
            image_source = std::make_shared<FileSource>(uri_source.uri.path());
        },
        [&](const fastgltf::sources::Array& array_source)
        {
            image_source = std::make_shared<MemorySource>(
                Buffer(array_source.bytes.data(), array_source.bytes.size()),
                SourceMetadata{
                    .source_id = create_name(ResourceType::Texture, index, texture_name.c_str()),
                    .resource_type = ResourceType::Texture,
                    .format = SourceFormat::Memory,
                    .size = array_source.bytes.size(),
                    .source_path = {}
                }
                );
        },
        [&](const fastgltf::sources::Vector& vector_source)
        {
            image_source = std::make_shared<MemorySource>(
                Buffer(vector_source.bytes.data(), vector_source.bytes.size()),
                SourceMetadata{
                    .source_id = create_name(ResourceType::Texture, index, texture_name.c_str()),
                    .resource_type = ResourceType::Texture,
                    .format = SourceFormat::Memory,
                    .size = vector_source.bytes.size(),
                    .source_path = {}
                }
                );
        },
        [&](const fastgltf::sources::BufferView& buffer_view_source)
        {
            auto& buffer_view = asset.bufferViews[buffer_view_source.bufferViewIndex];
            auto& buffer = asset.buffers[buffer_view.bufferIndex];
            std::visit(
                fastgltf::visitor{
                    [&](const fastgltf::sources::Array& array_buffer)
                    {
                        const auto* data_ptr = array_buffer.bytes.data() + buffer_view.byteOffset;
                        const auto data_size = buffer_view.byteLength;

                        image_source = std::make_shared<MemorySource>(
                            Buffer(data_ptr, data_size),
                            SourceMetadata{
                                .source_id = create_name(ResourceType::Texture, index, texture_name.c_str()),
                                .resource_type = ResourceType::Texture,
                                .format = SourceFormat::Memory,
                                .size = data_size,
                                .source_path = {}
                            }
                            );
                    },
                    [&](const fastgltf::sources::Vector& vector_buffer)
                    {
                        const auto* data_ptr = vector_buffer.bytes.data() + buffer_view.byteOffset;
                        const auto data_size = buffer_view.byteLength;

                        image_source = std::make_shared<MemorySource>(
                            Buffer(data_ptr, data_size),
                            SourceMetadata{
                                .source_id = create_name(ResourceType::Texture, index, texture_name.c_str()),
                                .resource_type = ResourceType::Texture,
                                .format = SourceFormat::Memory,
                                .size = data_size,
                                .source_path = {}
                            }
                            );
                    },
                    [&](auto&&)
                    {
                        LOGGER_ERROR("Unsupported buffer image source type");
                    }
                },
                buffer.data
                );
        }
    };
    std::visit(visitor, image.data);

    if (!image_source)
    {
        LOGGER_ERROR("Failed to create image source for texture: {}", texture_name);
        return;
    }

    texture_resource = registry->immediate_load(image_source).as<Texture>();

    auto sampler_index = texture.samplerIndex;
    PORTAL_ASSERT(sampler_index.has_value(), "Texture references invalid sampler: {}", texture_name);

    const auto sampler = asset.samplers[sampler_index.value()];
    const vk::SamplerCreateInfo sampler_info{
        .magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest)),
        .minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest)),
        .mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest)),
        .minLod = 0,
        .maxLod = vk::LodClampNone,
    };

    texture_resource->sampler = std::make_shared<vk::raii::Sampler>(gpu_context->create_sampler(sampler_info));
}

void GltfLoader::load_material(
    size_t index,
    const fastgltf::Asset& asset,
    const fastgltf::Material& material
    ) const
{
    static bool set_default = false;
    // TODO: move to material loader
    auto material_name = create_name(ResourceType::Material, index, material.name);
    auto resource = registry->get<Material>(material_name);

    if (resource->is_valid())
        return;

    std::vector<renderer::ShaderDefine> defines;

    const MaterialConsts consts{
        .color_factors = {
            material.pbrData.baseColorFactor[0],
            material.pbrData.baseColorFactor[1],
            material.pbrData.baseColorFactor[2],
            material.pbrData.baseColorFactor[3]
        },
        .metal_rough_factors = {
            material.pbrData.metallicFactor,
            material.pbrData.roughnessFactor,
            0.f,
            0.f
        }
    };
    resource->consts = consts;

    if (material.alphaMode == fastgltf::AlphaMode::Blend)
        resource->pass_type = MaterialPass::Transparent;
    else
        resource->pass_type = MaterialPass::MainColor;

    if (material.pbrData.baseColorTexture.has_value())
    {
        auto texture = asset.textures[material.pbrData.baseColorTexture.value().textureIndex];
        auto image = asset.images[texture.imageIndex.value()];
        auto texture_name = texture.name.empty() ? image.name : texture.name;

        auto image_ref = registry->get<Texture>(create_name(ResourceType::Texture, texture.imageIndex.value(), texture_name));

        resource->color_texture = image_ref;
    }
    else
    {
        resource->color_texture = registry->get<Texture>(Texture::BLACK_TEXTURE_ID);
    }

    PORTAL_ASSERT(resource->color_texture->is_valid(), "Material references invalid image: {} ", resource->color_texture->id);

    if (material.pbrData.metallicRoughnessTexture.has_value())
    {
        auto texture = asset.textures[material.pbrData.metallicRoughnessTexture.value().textureIndex];
        auto image = asset.images[texture.imageIndex.value()];
        auto texture_name = texture.name.empty() ? image.name : texture.name;

        auto image_ref = registry->get<Texture>(create_name(ResourceType::Texture, texture.imageIndex.value(), texture_name));

        resource->metallic_roughness_texture = image_ref;
    }
    else
    {
        resource->metallic_roughness_texture = registry->get<Texture>(Texture::WHITE_TEXTURE_ID);
    }

    auto shader_cache = registry->immediate_load<renderer::ShaderCache>(SHADER);
    auto hash = shader_cache->compile_with_permutations(defines);
    auto shader = shader_cache->get_shader(hash, gpu_context).lock();
    if (resource->pass_type == MaterialPass::Transparent)
        resource->pipeline = create_pipeline(STRING_ID("transparent_pipeline"), shader, false);
    else
        resource->pipeline = create_pipeline(STRING_ID("color_pipeline"), shader, true);

    auto& pipeline = resource->pipeline;

    auto global_descriptor_sets = gpu_context->get_global_descriptor_layouts();

    size_t skipped = 0;
    resource->descriptor_sets.clear();
    for (const auto& layout : pipeline->get_descriptor_set_layouts())
    {
        if (skipped < global_descriptor_sets.size())
        {
            skipped++;
            continue;
        }

        resource->descriptor_sets.push_back(gpu_context->create_descriptor_set(layout));
    }

    PORTAL_ASSERT(resource->pipeline->is_valid(), "Material references invalid pipeline: transparent_pipeline");

    auto vertex_shader = pipeline->get_shader(vk::ShaderStageFlagBits::eVertex);
    [[maybe_unused]] auto fragment_shader = pipeline->get_shader(vk::ShaderStageFlagBits::eFragment);
    PORTAL_ASSERT(vertex_shader == fragment_shader, "currently vertex and fragment are the same shader");

    {
        auto bind_context = vertex_shader->start_binding_context();

        resource->material_data = bind_context.make_buffers(gpu_context);

        bind_context.bind(STRING_ID("material_data.color_factors"), consts.color_factors);
        bind_context.bind(STRING_ID("material_data.metal_rough_factors"), consts.metal_rough_factors);
        bind_context.bind_texture(STRING_ID("material_data.color_texture"), resource->color_texture.lock());
        bind_context.bind_texture(STRING_ID("material_data.metal_rough_texture"), resource->metallic_roughness_texture.lock());

        bind_context.write_to_sets(resource->descriptor_sets, gpu_context);
    }

    resource->set_state(ResourceState::Loaded);

    if (!set_default)
    {
        set_default = true;
        registry->set_default(ResourceType::Material, resource);
    }
}

void GltfLoader::load_mesh(
    size_t index,
    const fastgltf::Asset& asset,
    const fastgltf::Mesh& mesh
    ) const
{
    //TODO: move to mesh loader
    auto mesh_resource = registry->get<Mesh>(create_name(ResourceType::Mesh, index, mesh.name.c_str()));

    for (auto&& p : mesh.primitives)
    {
        Surface surface{
            .start_index = static_cast<uint32_t>(mesh_resource->mesh_data.indices.size()),
            .count = static_cast<uint32_t>(asset.accessors[p.indicesAccessor.value()].count)
        };

        auto initial_vertex = static_cast<uint32_t>(mesh_resource->mesh_data.vertices.size());

        // load indexes
        {
            auto& index_accessor = asset.accessors[p.indicesAccessor.value()];
            mesh_resource->mesh_data.indices.reserve(mesh_resource->mesh_data.indices.size() + index_accessor.count);

            fastgltf::iterateAccessor<uint32_t>(
                asset,
                index_accessor,
                [&](const uint32_t& i)
                {
                    mesh_resource->mesh_data.indices.push_back(i + initial_vertex);
                }
                );
        }

        // load vertex positions
        {
            auto& position_accessor = asset.accessors[p.findAttribute("POSITION")->accessorIndex];
            mesh_resource->mesh_data.vertices.resize(mesh_resource->mesh_data.vertices.size() + position_accessor.count);

            fastgltf::iterateAccessorWithIndex<glm::vec3>(
                asset,
                position_accessor,
                [&](glm::vec3 v, size_t i)
                {
                    mesh_resource->mesh_data.vertices[initial_vertex + i] = {
                        .position = v,
                        .uv_x = 0,
                        .normal = {1, 0, 0},
                        .uv_y = 0,
                        .color = glm::vec4{1.f},
                    };
                }
                );
        }

        // load vertex normals
        auto normals = p.findAttribute("NORMAL");
        if (normals != p.attributes.end())
        {
            fastgltf::iterateAccessorWithIndex<glm::vec3>(
                asset,
                asset.accessors[normals->accessorIndex],
                [&](glm::vec3 v, size_t i)
                {
                    mesh_resource->mesh_data.vertices[initial_vertex + i].normal = v;
                }
                );
        }

        // load UVs
        auto uv = p.findAttribute("TEXCOORD_0");
        if (uv != p.attributes.end())
        {

            fastgltf::iterateAccessorWithIndex<glm::vec2>(
                asset,
                asset.accessors[uv->accessorIndex],
                [&](glm::vec2 v, size_t i)
                {
                    mesh_resource->mesh_data.vertices[initial_vertex + i].uv_x = v.x;
                    mesh_resource->mesh_data.vertices[initial_vertex + i].uv_y = v.y;
                }
                );
        }

        // load vertex colors
        auto colors = p.findAttribute("COLOR_0");
        if (colors != p.attributes.end())
        {

            fastgltf::iterateAccessorWithIndex<glm::vec4>(
                asset,
                asset.accessors[colors->accessorIndex],
                [&](glm::vec4 v, size_t i)
                {
                    mesh_resource->mesh_data.vertices[initial_vertex + i].color = v;
                }
                );
        }

        if (p.materialIndex.has_value())
        {
            auto& material = asset.materials[p.materialIndex.value()];
            surface.material = registry->get<Material>(create_name(ResourceType::Material, p.materialIndex.value(), material.name.c_str()));
        }
        else
        {
            // TODO: put something here
            surface.material = registry->get<Material>(STRING_ID("default_material"));
        }

        //loop the vertices of this surface, find min/max bounds
        glm::vec3 min_pos = mesh_resource->mesh_data.vertices[initial_vertex].position;
        glm::vec3 max_pos = mesh_resource->mesh_data.vertices[initial_vertex].position;
        for (size_t i = initial_vertex; i < mesh_resource->mesh_data.vertices.size(); ++i)
        {
            min_pos = glm::min(min_pos, mesh_resource->mesh_data.vertices[i].position);
            max_pos = glm::max(max_pos, mesh_resource->mesh_data.vertices[i].position);
        }

        // calculate origin and extents from the min/max, use extent lenght for radius
        surface.bounds.origin = (max_pos + min_pos) / 2.f;
        surface.bounds.extents = (max_pos - min_pos) / 2.f;
        surface.bounds.sphere_radius = glm::length(surface.bounds.extents);

        mesh_resource->surfaces.push_back(surface);
    }

    // Create mesh buffers
    const size_t vertex_buffer_size = mesh_resource->mesh_data.vertices.size() * sizeof(Vertex);
    const size_t index_buffer_size = mesh_resource->mesh_data.indices.size() * sizeof(uint32_t);

    renderer::vulkan::BufferBuilder vertex_builder{vertex_buffer_size};
    vertex_builder.with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
                  .with_usage(
                      vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress
                      | vk::BufferUsageFlagBits::eTransferSrc
                      )
                  .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY);

    renderer::vulkan::BufferBuilder index_builder{index_buffer_size};
    index_builder.with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
                 .with_usage(
                     vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc
                     )
                 .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY);

    mesh_resource->mesh_data.index_buffer = gpu_context->create_buffer_shared(index_builder);
    mesh_resource->mesh_data.vertex_buffer = gpu_context->create_buffer_shared(vertex_builder);
    mesh_resource->mesh_data.vertex_buffer_address = mesh_resource->mesh_data.vertex_buffer->get_device_address();

    renderer::vulkan::BufferBuilder builder(vertex_buffer_size + index_buffer_size);
    builder.with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
           .with_usage(vk::BufferUsageFlagBits::eTransferSrc)
           .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU)
           .with_debug_name("staging");
    auto staging_buffer = gpu_context->create_buffer(builder);
    auto offset = staging_buffer.update(mesh_resource->mesh_data.vertices.data(), vertex_buffer_size, 0);
    offset += staging_buffer.update(mesh_resource->mesh_data.indices.data(), index_buffer_size, offset);

    gpu_context->immediate_submit(
        [&](const vk::raii::CommandBuffer& command_buffer)
        {
            vk::BufferCopy vertex_copy{
                .srcOffset = 0,
                .dstOffset = 0,
                .size = vertex_buffer_size
            };
            command_buffer.copyBuffer(
                staging_buffer.get_handle(),
                mesh_resource->mesh_data.vertex_buffer->get_handle(),
                {vertex_copy}
                );

            vk::BufferCopy index_copy{
                .srcOffset = vertex_buffer_size,
                .dstOffset = 0,
                .size = index_buffer_size
            };
            command_buffer.copyBuffer(
                staging_buffer.get_handle(),
                mesh_resource->mesh_data.index_buffer->get_handle(),
                {index_copy}
                );
        }
        );
}

std::vector<Ref<Scene>> GltfLoader::load_scenes(const fastgltf::Asset& asset) const
{
    std::vector<Ref<scene::Node>> nodes;
    int node_index = 0;
    for (auto& node : asset.nodes)
    {
        Ref<scene::Node> new_node = nullptr;

        auto node_name = STRING_ID(fmt::format("node{}-{}", node_index, node.name));
        if (node.meshIndex.has_value())
        {
            new_node = Ref<scene::MeshNode>::create(node_name);
            new_node.as<scene::MeshNode>()->mesh = registry->get<Mesh>(
                create_name(ResourceType::Mesh, node.meshIndex.value(), asset.meshes[node.meshIndex.value()].name.c_str())
                );
        }
        else
        {
            new_node = Ref<scene::Node>::create(node_name);
        }

        std::visit(
            fastgltf::visitor{[&](fastgltf::math::fmat4x4 matrix)
                              {
                                  std::memcpy(&new_node->local_transform, matrix.data(), sizeof(matrix));
                              },
                              [&](fastgltf::TRS transform)
                              {
                                  glm::vec3 tl(
                                      transform.translation[0],
                                      transform.translation[1],
                                      transform.translation[2]
                                      );
                                  glm::quat rot(
                                      transform.rotation[3],
                                      transform.rotation[0],
                                      transform.rotation[1],
                                      transform.rotation[2]
                                      );
                                  glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

                                  glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
                                  glm::mat4 rm = glm::toMat4(rot);
                                  glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

                                  new_node->local_transform = tm * rm * sm;
                              }},
            node.transform
            );

        nodes.push_back(new_node);
        node_index++;
    }

    for (size_t i = 0; i < asset.nodes.size(); i++)
    {
        const fastgltf::Node& node = asset.nodes[i];
        auto& scene_node = nodes[i];

        for (auto& c : node.children)
        {
            scene_node->children.push_back(nodes[c]);
            nodes[c]->parent = scene_node;
        }
    }

    std::vector<Ref<Scene>> scenes;
    size_t index = 0;
    for (const auto& [nodeIndices, name] : asset.scenes)
    {
        auto scene_resource = registry->get<Scene>(create_name(ResourceType::Scene, index, name.c_str()));

        for (auto& scene_node_index : nodeIndices)
        {
            auto& node = nodes[scene_node_index];
            if (node->parent.lock() == nullptr)
            {
                scene_resource->add_root_node(node);
                node->refresh_transform(glm::mat4{1.f});
            }
        }

        scene_resource->set_state(ResourceState::Loaded);
        scenes.push_back(scene_resource);
        index++;
    }
    return scenes;
}

Ref<renderer::Pipeline> GltfLoader::create_pipeline(const StringId& name, const Ref<renderer::Shader>& shader, const bool depth) const
{
    // TODO: add pipeline cache
    renderer::pipeline::Specification pipeline_spec {
        .shader = shader,
        .render_target = gpu_context->get_render_target(),
        .topology = renderer::PrimitiveTopology::Triangles,
        .depth_compare_operator = renderer::DepthCompareOperator::GreaterOrEqual,
        .backface_culling = false,
        .depth_test = depth,
        .depth_write = depth,
        .wireframe = false,
        .debug_name = name
    };
    return Ref<renderer::vulkan::VulkanPipeline>::create(pipeline_spec, gpu_context);
}

} // portal
