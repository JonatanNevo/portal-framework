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
#include "portal/engine/renderer/shaders/shader_types.h"
#include "portal/engine/resources/resource_registry.h"
#include "portal/engine/resources/loader/loader_factory.h"
#include "portal/engine/resources/resources/mesh.h"
#include "portal/engine/renderer/vulkan/vulkan_pipeline.h"
#include "portal/engine/resources/source/file_source.h"
#include "portal/engine/resources/source/memory_source.h"
#include "portal/engine/resources/source/resource_source.h"
#include "portal/engine/scene/scene.h"
#include "portal/engine/scene/nodes/mesh_node.h"
#include "portal/engine/scene/nodes/node.h"
#include "portal/engine/renderer/vulkan/gpu_context.h"
#include "portal/engine/renderer/vulkan/vulkan_device.h"
#include "portal/engine/renderer/vulkan/vulkan_material.h"
#include "portal/engine/renderer/vulkan/vulkan_shader.h"
#include "portal/engine/renderer/vulkan/image/vulkan_texture.h"


namespace portal::resources
{

static auto logger = Log::get_logger("Resources");
// const auto SHADER = "mesh.shading.slang.spv";
const auto SHADER = STRING_ID("pbr.slang");

renderer::TextureFilter extract_filter(const fastgltf::Filter filter)
{
    switch (filter)
    {
    case fastgltf::Filter::Nearest:
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::NearestMipMapLinear:
        return renderer::TextureFilter::Nearest;
    case fastgltf::Filter::Linear:
    case fastgltf::Filter::LinearMipMapLinear:
    case fastgltf::Filter::LinearMipMapNearest:
        return renderer::TextureFilter::Linear;
    }
    return renderer::TextureFilter::Linear;
}

renderer::SamplerMipmapMode extract_mipmap_mode(const fastgltf::Filter filter)
{
    switch (filter)
    {
    case fastgltf::Filter::Nearest:
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::LinearMipMapNearest:
        return renderer::SamplerMipmapMode::Nearest;
    case fastgltf::Filter::Linear:
    case fastgltf::Filter::NearestMipMapLinear:
    case fastgltf::Filter::LinearMipMapLinear:
        return renderer::SamplerMipmapMode::Linear;
    }
    return renderer::SamplerMipmapMode::Linear;
}

StringId create_name(ResourceType type, size_t index, std::string_view name)
{
    return STRING_ID(fmt::format("{}{}-{}", type, index, name));
}

GltfLoader::GltfLoader(ResourceRegistry* registry, const std::shared_ptr<renderer::vulkan::GpuContext>& context) : ResourceLoader(registry),
    gpu_context(context) {}

bool GltfLoader::load(StringId id, const std::shared_ptr<ResourceSource> source) const
{
    PORTAL_PROF_ZONE();

    const auto data = source->load();
    auto data_result = fastgltf::GltfDataBuffer::FromBytes(data.as<std::byte*>(), data.size);
    if (!data_result)
    {
        LOGGER_ERROR("Failed to load glTF from: {}, error: {}", id, fastgltf::getErrorMessage(data_result.error()));
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

    auto& root_resource = registry->get(id, ResourceType::Scene);
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

    auto texture_resource = registry->get<renderer::Texture>(create_name(ResourceType::Texture, index, texture_name.c_str()));
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

    auto vulkan_texture = registry->immediate_load(create_name(ResourceType::Texture, index, texture_name.c_str()), image_source).as<
        renderer::vulkan::VulkanTexture>();

    auto sampler_index = texture.samplerIndex;
    PORTAL_ASSERT(sampler_index.has_value(), "Texture references invalid sampler: {}", texture_name);

    const auto sampler = asset.samplers[sampler_index.value()];
    renderer::SamplerSpecification sampler_spec = {
        .filter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest)),
        .mipmap_mode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest)),
        .min_lod = 0,
        .max_lod = vk::LodClampNone,
    };

    const auto sampler_ref = Ref<renderer::vulkan::VulkanSampler>::create(
        STRING_ID(fmt::format("{}-sampler", image.name)),
        sampler_spec,
        gpu_context->get_context()->get_device()
        );
    vulkan_texture->set_sampler(sampler_ref);
}

enum class MaterialPass
{
    Transparent,
    MainColor
};

void GltfLoader::load_material(
    size_t index,
    const fastgltf::Asset& asset,
    const fastgltf::Material& gltf_material
    ) const
{
    static bool set_default = false;
    // TODO: move to material loader
    auto material_name = create_name(ResourceType::Material, index, gltf_material.name);
    auto resource = registry->get<renderer::Material>(material_name);

    if (resource->is_valid())
        return;

    std::vector<renderer::ShaderDefine> defines;

    glm::vec4 color_factors = {
        gltf_material.pbrData.baseColorFactor[0],
        gltf_material.pbrData.baseColorFactor[1],
        gltf_material.pbrData.baseColorFactor[2],
        gltf_material.pbrData.baseColorFactor[3]
    };

    glm::vec4 metallic_factors = {
        gltf_material.pbrData.metallicFactor,
        gltf_material.pbrData.roughnessFactor,
        0.f,
        0.f
    };

    MaterialPass pass_type;
    if (gltf_material.alphaMode == fastgltf::AlphaMode::Blend)
        pass_type = MaterialPass::Transparent;
    else
        pass_type = MaterialPass::MainColor;

    auto shader_cache = registry->immediate_load<renderer::vulkan::VulkanShader>(SHADER);
    auto hash = shader_cache->compile_with_permutations(defines);
    auto shader = shader_cache->get_shader(hash).lock();

    auto global_descriptor_sets = gpu_context->get_global_descriptor_layouts();

    renderer::MaterialSpecification spec{
        .id = material_name,
        .shader = shader,
        .set_start_index = global_descriptor_sets.size(),
        .default_texture = registry->get<renderer::Texture>(renderer::Texture::MISSING_TEXTURE_ID),
    };

    auto material = resource.as<renderer::vulkan::VulkanMaterial>();
    material->initialize(spec, gpu_context->get_context());

    if (gltf_material.pbrData.baseColorTexture.has_value())
    {
        auto texture = asset.textures[gltf_material.pbrData.baseColorTexture.value().textureIndex];
        auto image = asset.images[texture.imageIndex.value()];
        auto texture_name = texture.name.empty() ? image.name : texture.name;

        auto image_ref = registry->get<renderer::Texture>(create_name(ResourceType::Texture, texture.imageIndex.value(), texture_name));

        material->set(STRING_ID("material_data.color_texture"), image_ref);
    }
    else
    {
        material->set(STRING_ID("material_data.color_texture"), registry->get<renderer::Texture>(renderer::Texture::WHITE_TEXTURE_ID));
    }

    if (gltf_material.pbrData.metallicRoughnessTexture.has_value())
    {
        auto texture = asset.textures[gltf_material.pbrData.metallicRoughnessTexture.value().textureIndex];
        auto image = asset.images[texture.imageIndex.value()];
        auto texture_name = texture.name.empty() ? image.name : texture.name;

        auto image_ref = registry->get<renderer::Texture>(create_name(ResourceType::Texture, texture.imageIndex.value(), texture_name));

        material->set(STRING_ID("material_data.metal_rough_texture"), image_ref);
    }
    else
    {
        material->set(STRING_ID("material_data.metal_rough_texture"), registry->get<renderer::Texture>(renderer::Texture::WHITE_TEXTURE_ID));
    }

    if (pass_type == MaterialPass::Transparent)
        material->set_pipeline(create_pipeline(STRING_ID("transparent_pipeline"), shader, false));
    else
        material->set_pipeline(create_pipeline(STRING_ID("color_pipeline"), shader, true));

    material->set(STRING_ID("material_data.color_factors"), color_factors);
    material->set(STRING_ID("material_data.metal_rough_factors"), metallic_factors);

    material->set_state(ResourceState::Loaded);

    if (!set_default)
    {
        set_default = true;
        registry->set_default(ResourceType::Material, material);
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
            surface.material = registry->get<renderer::Material>(create_name(ResourceType::Material, p.materialIndex.value(), material.name.c_str()));
        }
        else
        {
            // TODO: put something here
            surface.material = registry->get<renderer::Material>(STRING_ID("default_material"));
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

    mesh_resource->mesh_data.index_buffer = gpu_context->get_context()->get_device()->create_buffer_shared(index_builder);
    mesh_resource->mesh_data.vertex_buffer = gpu_context->get_context()->get_device()->create_buffer_shared(vertex_builder);
    mesh_resource->mesh_data.vertex_buffer_address = mesh_resource->mesh_data.vertex_buffer->get_device_address();

    renderer::vulkan::BufferBuilder builder(vertex_buffer_size + index_buffer_size);
    builder.with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
           .with_usage(vk::BufferUsageFlagBits::eTransferSrc)
           .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU)
           .with_debug_name("staging");
    auto staging_buffer = gpu_context->get_context()->get_device()->create_buffer(builder);
    auto offset = staging_buffer.update(mesh_resource->mesh_data.vertices.data(), vertex_buffer_size, 0);
    offset += staging_buffer.update(mesh_resource->mesh_data.indices.data(), index_buffer_size, offset);

    gpu_context->get_context()->get_device()->immediate_submit(
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

static Ref<renderer::vulkan::VulkanPipeline> g_transparent_pipeline;
static Ref<renderer::vulkan::VulkanPipeline> g_color_pipeline;

Ref<renderer::Pipeline> GltfLoader::create_pipeline(const StringId& name, const Ref<renderer::ShaderVariant>& shader, const bool depth) const
{
    if (name == STRING_ID("transparent_pipeline") && g_transparent_pipeline != nullptr)
        return g_transparent_pipeline;

    if (name == STRING_ID("color_pipeline") && g_color_pipeline != nullptr)
        return g_color_pipeline;

    // TODO: add pipeline cache
    renderer::pipeline::Specification pipeline_spec{
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
    auto pipeline = Ref<renderer::vulkan::VulkanPipeline>::create(pipeline_spec, gpu_context->get_context());

    if (name == STRING_ID("color_pipeline"))
        g_color_pipeline = pipeline;

    if (name == STRING_ID("transparent_pipeline"))
        g_transparent_pipeline = pipeline;

    return pipeline;
}

} // portal
