//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "gltf_loader.h"

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "portal/core/debug/profile.h"
#include "portal/application/settings.h"
#include "portal/engine/renderer/renderer_context.h"
#include "portal/engine/renderer/shaders/shader_types.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/engine/resources/resource_registry.h"
#include "portal/engine/resources/loader/loader_factory.h"
#include "portal/engine/resources/resources/mesh_geometry.h"
#include "portal/engine/resources/loader/material_loader.h"
#include "portal/engine/resources/source/memory_source.h"
#include "portal/engine/resources/source/resource_source.h"
#include "portal/engine/renderer/vulkan/vulkan_material.h"
#include "portal/engine/renderer/vulkan/image/vulkan_texture.h"
#include "portal/engine/resources/loader/mesh_loader.h"
#include "portal/engine/resources/loader/scene_loader.h"
#include "portal/engine/resources/resources/composite.h"
#include "portal/serialization/serialize/binary_serialization.h"


namespace portal::resources
{
static auto logger = Log::get_logger("Resources");

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

std::string create_name_relative(const std::filesystem::path path, const auto& part, const ResourceType type)
{
    return (path / fmt::format("gltf-{}-{}", to_string(type), part)).generic_string();
}

GltfLoader::GltfLoader(ResourceRegistry& registry, const RendererContext& context) : ResourceLoader(registry), context(context)
{
}

void GltfLoader::enrich_metadata(SourceMetadata& meta, const ResourceSource& source)
{
    CompositeMetadata composite_meta;

    const auto parent_path = std::filesystem::path(meta.source.string).parent_path();
    const auto base_name = std::filesystem::path(meta.resource_id.string).parent_path();
    auto create_name = [&base_name](const auto& part, const ResourceType type)
    {
        return create_name_relative(base_name, part, type);
    };

    // TODO: use enum
    composite_meta.type = "glTF";

    const auto data = source.load();
    auto data_result = fastgltf::GltfDataBuffer::FromBytes(data.as<std::byte*>(), data.size);
    if (!data_result)
    {
        LOGGER_ERROR("Failed to load glTF from: {}, error: {}", meta.resource_id, fastgltf::getErrorMessage(data_result.error()));
        return;
    }

    const auto gltf = load_asset(meta, data_result.get());

    for (auto& texture : gltf.textures)
    {
        auto image_index = texture.imageIndex;
        const auto& [_, name] = gltf.images[image_index.value()];
        auto texture_name = texture.name.empty() ? name : texture.name;

        auto [image_meta, image_source] = find_image_source(base_name, parent_path, gltf, texture);
        if (image_source)
        {
            LoaderFactory::enrich_metadata(image_meta, *image_source);
            composite_meta.children[std::string(image_meta.resource_id.string)] = meta;
        }
        meta.dependencies.push_back(image_meta.resource_id);
    }

    for (auto& material : gltf.materials)
    {
        llvm::SmallVector<StringId> dependencies{};
        if (material.pbrData.baseColorTexture.has_value())
        {
            auto texture = gltf.textures[material.pbrData.baseColorTexture.value().textureIndex];
            auto [texture_meta, _] = find_image_source(base_name, parent_path, gltf, texture);
            dependencies.push_back(texture_meta.resource_id);
        }
        if (material.pbrData.metallicRoughnessTexture.has_value())
        {
            auto texture = gltf.textures[material.pbrData.metallicRoughnessTexture.value().textureIndex];
            auto [texture_meta, _] = find_image_source(base_name, parent_path, gltf, texture);
            dependencies.push_back(texture_meta.resource_id);
        }

        auto resource_id = STRING_ID(create_name(material.name, ResourceType::Material));
        SourceMetadata material_meta{
            .resource_id = resource_id,
            .type = ResourceType::Material,
            .dependencies = dependencies,
            .source = STRING_ID(fmt::format("mem://gltf-material/{}", material.name)),
            .format = SourceFormat::Memory
        };

        // TODO: create sub source
        LoaderFactory::enrich_metadata(material_meta, source);

        composite_meta.children[create_name(material.name, ResourceType::Material)] = material_meta;
        meta.dependencies.push_back(resource_id);
    }

    for (auto& mesh : gltf.meshes)
    {
        llvm::SmallVector<StringId> dependencies{};

        for (const auto& p : mesh.primitives)
        {
            if (p.materialIndex.has_value())
            {
                auto& material = gltf.materials[p.materialIndex.value()];
                dependencies.push_back(STRING_ID(create_name(material.name, ResourceType::Material)));
            }
        }

        auto resource_id = STRING_ID(create_name(mesh.name, ResourceType::Mesh));

        SourceMetadata mesh_meta = {
            .resource_id = resource_id,
            .type = ResourceType::Mesh,
            .dependencies = dependencies,
            .source = STRING_ID(fmt::format("mem://gltf-mesh/{}", mesh.name.c_str())),
            .format = SourceFormat::Memory
        };

        LoaderFactory::enrich_metadata(mesh_meta, source);

        composite_meta.children[create_name(mesh.name, ResourceType::Mesh)] = mesh_meta;
        meta.dependencies.push_back(resource_id);
    }

    std::function<void(llvm::SmallVector<StringId>&, const size_t&)> add_node_to_dependencies;

    add_node_to_dependencies = [&](llvm::SmallVector<StringId>& dependencies, const auto& node_index)
    {
        const fastgltf::Node& node = gltf.nodes[node_index];
        if (node.meshIndex.has_value())
        {
            const auto& mesh = gltf.meshes[node.meshIndex.value()];
            dependencies.push_back(STRING_ID(create_name(mesh.name, ResourceType::Mesh)));
        }

        for (const auto index : node.children)
        {
            add_node_to_dependencies(dependencies, index);
        }
    };

    for (const auto& scene : gltf.scenes)
    {
        llvm::SmallVector<StringId> dependencies{};

        for (auto& index : scene.nodeIndices)
            add_node_to_dependencies(dependencies, index);

        auto resource_id = STRING_ID(create_name(scene.name, ResourceType::Scene));
        SourceMetadata mesh_meta = {
            .resource_id = resource_id,
            .type = ResourceType::Scene,
            .dependencies = dependencies,
            .source = STRING_ID(fmt::format("mem://gltf-scene/{}", scene.name)),
            .format = SourceFormat::Memory
        };

        LoaderFactory::enrich_metadata(mesh_meta, source);

        composite_meta.children[create_name(scene.name, ResourceType::Scene)] = mesh_meta;
        meta.dependencies.push_back(resource_id);
    }

    meta.meta = std::move(composite_meta);
}

Reference<Resource> GltfLoader::load(const SourceMetadata& meta, const ResourceSource& source)
{
    PORTAL_PROF_ZONE();

    const auto parent_path = std::filesystem::path(meta.resource_id.string).parent_path();
    auto relative_name = [&parent_path](const auto& part, ResourceType type)
    {
        return create_name_relative(parent_path, part, type);
    };

    const auto data = source.load();
    auto data_result = fastgltf::GltfDataBuffer::FromBytes(data.as<std::byte*>(), data.size);
    if (!data_result)
    {
        LOGGER_ERROR("Failed to load glTF from: {}, error: {}", meta.resource_id, fastgltf::getErrorMessage(data_result.error()));
        return nullptr;
    }

    const auto gltf = load_asset(meta, data_result.get());

    auto composite_meta = std::get<CompositeMetadata>(meta.meta);

    llvm::SmallVector<Job<>> texture_jobs{};
    texture_jobs.reserve(gltf.textures.size());
    for (auto& texture : gltf.textures)
    {
        auto texture_name = relative_name(texture.name.c_str(), ResourceType::Texture);
        if (composite_meta.children.contains(texture_name))
        {
            auto texture_meta = composite_meta.children.at(texture_name);
            texture_jobs.emplace_back(load_texture(texture_meta, gltf, texture));
        }
    }
    registry.wait_all(texture_jobs);

    llvm::SmallVector<Job<>> material_jobs{};
    material_jobs.reserve(gltf.materials.size());
    for (auto& material : gltf.materials)
    {
        auto material_metadata = composite_meta.children.at(relative_name(material.name.c_str(), ResourceType::Material));
        material_jobs.emplace_back(load_material(material_metadata, gltf, material));
    }
    registry.wait_all(material_jobs);


    llvm::SmallVector<Job<>> mesh_jobs{};
    mesh_jobs.reserve(gltf.materials.size());
    for (auto& mesh : gltf.meshes)
    {
        auto mesh_metadata = composite_meta.children.at(relative_name(mesh.name.c_str(), ResourceType::Mesh));
        mesh_jobs.emplace_back(load_mesh(mesh_metadata, gltf, mesh));
    }
    registry.wait_all(mesh_jobs);

    load_scenes(meta, gltf);


    auto composite = make_reference<Composite>(meta.resource_id);
    for (auto& child_meta : composite_meta.children | std::views::values)
    {
        auto resource = registry.get<Resource>(child_meta.resource_id);
        if (resource.get_state() != ResourceState::Loaded)
            LOGGER_ERROR("Failed to load resource: {}", child_meta.resource_id);

        composite->set_resource(child_meta.type, child_meta.resource_id, resource);
    }

    return composite;
}

fastgltf::Asset GltfLoader::load_asset(const SourceMetadata& meta, fastgltf::GltfDataGetter& data)
{
    const auto parent_path = std::filesystem::path(meta.full_source_path.string).parent_path();

    constexpr auto glft_options = fastgltf::Options::DontRequireValidAssetMember
        | fastgltf::Options::AllowDouble
        | fastgltf::Options::LoadExternalBuffers;

    fastgltf::Parser parser;

    auto load = parser.loadGltf(data, parent_path, glft_options);
    if (!load)
    {
        LOGGER_ERROR("Failed to load glTF {}, error: {}", meta.resource_id, fastgltf::getErrorMessage(load.error()));
        return fastgltf::Asset{};
    }

    return std::move(load.get());
}

std::pair<SourceMetadata, std::unique_ptr<ResourceSource>> GltfLoader::find_image_source(
    const std::filesystem::path& base_name,
    const std::filesystem::path& base_path,
    const fastgltf::Asset& asset,
    const fastgltf::Texture& texture
)
{
    std::unique_ptr<ResourceSource> image_source = nullptr;
    SourceMetadata image_source_meta{};

    auto image_index = texture.imageIndex;
    PORTAL_ASSERT(image_index.has_value(), "Texture references invalid image: {}", texture.name);
    const auto& [data, name] = asset.images[image_index.value()];
    auto texture_name = texture.name.empty() ? name : texture.name;

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
            image_source = nullptr;
            auto uri_path = uri_source.uri.path();
            auto uri_filename = std::filesystem::path(uri_path).stem();

            image_source_meta = SourceMetadata{
                .resource_id = STRING_ID((base_name / uri_filename).generic_string()),
                .type = ResourceType::Texture,
                .source = STRING_ID((base_path / std::filesystem::path(uri_source.uri.path())).generic_string()),
                .format = SourceFormat::Image,
            };
        },
        [&](const fastgltf::sources::Array& array_source)
        {
            image_source_meta = SourceMetadata{
                .resource_id = STRING_ID(create_name_relative(base_name, texture_name, ResourceType::Texture)),
                .type = ResourceType::Texture,
                .source = STRING_ID(fmt::format("mem://gltf-texture/array/{}", texture_name)),
                .format = SourceFormat::Memory,
            };

            image_source = std::make_unique<MemorySource>(Buffer(array_source.bytes.data(), array_source.bytes.size()));
        },
        [&](const fastgltf::sources::Vector& vector_source)
        {
            image_source_meta = SourceMetadata{
                .resource_id = STRING_ID(create_name_relative(base_name, texture_name, ResourceType::Texture)),
                .type = ResourceType::Texture,
                .source = STRING_ID(fmt::format("mem://gltf-texture/vector/{}", texture_name)),
                .format = SourceFormat::Memory,
            };

            image_source = std::make_unique<MemorySource>(Buffer(vector_source.bytes.data(), vector_source.bytes.size()));
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

                        image_source_meta = SourceMetadata{
                            .resource_id = STRING_ID(create_name_relative(base_name, texture_name, ResourceType::Texture)),
                            .type = ResourceType::Texture,
                            .source = STRING_ID(fmt::format("mem://gltf-texture/view/array/{}", texture_name)),
                            .format = SourceFormat::Memory,
                        };

                        image_source = std::make_unique<MemorySource>(Buffer(data_ptr, data_size));
                    },
                    [&](const fastgltf::sources::Vector& vector_buffer)
                    {
                        const auto* data_ptr = vector_buffer.bytes.data() + buffer_view.byteOffset;
                        const auto data_size = buffer_view.byteLength;

                        image_source_meta = SourceMetadata{
                            .resource_id = STRING_ID(create_name_relative(base_name, texture_name, ResourceType::Texture)),
                            .type = ResourceType::Texture,
                            .source = STRING_ID(fmt::format("mem://gltf-texture/view/vector/{}", texture_name)),
                            .format = SourceFormat::Memory,
                        };

                        image_source = std::make_unique<MemorySource>(Buffer(data_ptr, data_size));
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
    std::visit(visitor, data);

    return {image_source_meta, std::move(image_source)};
}

Job<> GltfLoader::load_texture(
    SourceMetadata texture_meta,
    const fastgltf::Asset& asset,
    const fastgltf::Texture& texture
) const
{
    const auto parent_path = std::filesystem::path(texture_meta.source.string).parent_path();
    const auto base_name = std::filesystem::path(texture_meta.resource_id.string).parent_path();
    const auto texture_name = texture_meta.resource_id;
    if (registry.get<renderer::vulkan::VulkanTexture>(texture_name).get_state() == ResourceState::Loaded)
        co_return;

    const auto result = find_image_source(base_name, parent_path, asset, texture);
    auto& image_meta = result.first;
    auto& source = result.second;
    if (!source)
        co_return;

    auto job = registry.load_direct(image_meta, *source);
    co_await job;
    auto res = job.result();
    if (!res)
    {
        LOGGER_ERROR("Failed to load image source for texture: {}", texture_name);
        co_return;
    }

    const auto texture_resource = res.value();
    const auto vulkan_texture = reference_cast<renderer::vulkan::VulkanTexture>(texture_resource);

    auto sampler_index = texture.samplerIndex;
    PORTAL_ASSERT(sampler_index.has_value(), "Texture references invalid sampler: {}", texture_name);

    const auto sampler = asset.samplers[sampler_index.value()];
    renderer::SamplerProperties sampler_prop = {
        .filter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest)),
        .mipmap_mode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest)),
        .min_lod = 0,
        .max_lod = vk::LodClampNone,
    };

    const auto sampler_ref = make_reference<renderer::vulkan::VulkanSampler>(
        STRING_ID(fmt::format("{}-sampler", texture_meta.resource_id.string)),
        sampler_prop,
        context.get_gpu_context().get_device()
    );
    vulkan_texture->set_sampler(sampler_ref);
}

Job<> GltfLoader::load_material(
    SourceMetadata material_meta,
    const fastgltf::Asset& asset,
    const fastgltf::Material& material
) const
{
    const auto base_name = std::filesystem::path(material_meta.resource_id.string).parent_path();
    const auto parent_path = std::filesystem::path(material_meta.source.string).parent_path();

    const auto& material_name = material_meta.resource_id;
    if (registry.get<renderer::vulkan::VulkanMaterial>(material_name).get_state() == ResourceState::Loaded)
        co_return;

    MaterialDetails details{
        .color_factors = {
            material.pbrData.baseColorFactor[0],
            material.pbrData.baseColorFactor[1],
            material.pbrData.baseColorFactor[2],
            material.pbrData.baseColorFactor[3]
        },
        .metallic_factors = {
            material.pbrData.metallicFactor,
            material.pbrData.roughnessFactor,
            0.f,
            0.f
        }
    };

    if (material.alphaMode == fastgltf::AlphaMode::Blend)
        details.pass_type = MaterialPass::Transparent;
    else
        details.pass_type = MaterialPass::MainColor;

    if (material.pbrData.baseColorTexture.has_value())
    {
        auto texture = asset.textures[material.pbrData.baseColorTexture.value().textureIndex];
        auto [texture_meta, _] = find_image_source(base_name, parent_path, asset, texture);
        details.color_texture = texture_meta.resource_id;
    }

    if (material.pbrData.metallicRoughnessTexture.has_value())
    {
        auto texture = asset.textures[material.pbrData.metallicRoughnessTexture.value().textureIndex];
        auto [texture_meta, _] = find_image_source(base_name, parent_path, asset, texture);
        details.metallic_texture = texture_meta.resource_id;
    }

    MemorySource source{Buffer(&details, sizeof(details))};
    co_await registry.load_direct(material_meta, source);
}

Job<> GltfLoader::load_mesh(
    SourceMetadata mesh_meta,
    const fastgltf::Asset& asset,
    const fastgltf::Mesh& mesh
) const
{
    const auto parent_path = std::filesystem::path(mesh_meta.resource_id.string).parent_path();

    if (registry.get<MeshGeometry>(mesh_meta.resource_id).get_state() == ResourceState::Loaded)
        co_return;

    MeshData mesh_data;
    mesh_data.submeshes.reserve(mesh.primitives.size());

    for (const auto& p : mesh.primitives)
    {
        MeshGeometryData::Submesh submesh{
            .start_index = static_cast<uint32_t>(mesh_data.indices.size()),
            .count = static_cast<uint32_t>(asset.accessors[p.indicesAccessor.value()].count)
        };

        auto initial_vertex = static_cast<uint32_t>(mesh_data.vertices.size());

        // load indexes
        {
            auto& index_accessor = asset.accessors[p.indicesAccessor.value()];
            mesh_data.indices.reserve(mesh_data.indices.size() + index_accessor.count);

            fastgltf::iterateAccessor<uint32_t>(
                asset,
                index_accessor,
                [&mesh_data, initial_vertex](const uint32_t& i)
                {
                    mesh_data.indices.push_back(i + initial_vertex);
                }
            );
        }

        // load vertex positions
        {
            auto& position_accessor = asset.accessors[p.findAttribute("POSITION")->accessorIndex];
            mesh_data.vertices.resize(mesh_data.vertices.size() + position_accessor.count);

            fastgltf::iterateAccessorWithIndex<glm::vec3>(
                asset,
                position_accessor,
                [&mesh_data, initial_vertex](glm::vec3 v, size_t i)
                {
                    mesh_data.vertices[initial_vertex + i] = {
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
                [&mesh_data, initial_vertex](glm::vec3 v, size_t i)
                {
                    mesh_data.vertices[initial_vertex + i].normal = v;
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
                [&mesh_data, initial_vertex](glm::vec2 v, size_t i)
                {
                    mesh_data.vertices[initial_vertex + i].uv_x = v.x;
                    mesh_data.vertices[initial_vertex + i].uv_y = v.y;
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
                [&mesh_data, initial_vertex](glm::vec4 v, size_t i)
                {
                    mesh_data.vertices[initial_vertex + i].color = v;
                }
            );
        }

        //loop the vertices of this surface, find min/max bounds
        glm::vec3 min_pos = mesh_data.vertices[initial_vertex].position;
        glm::vec3 max_pos = mesh_data.vertices[initial_vertex].position;
        for (size_t i = initial_vertex; i < mesh_data.vertices.size(); ++i)
        {
            min_pos = glm::min(min_pos, mesh_data.vertices[i].position);
            max_pos = glm::max(max_pos, mesh_data.vertices[i].position);
        }

        // calculate origin and extents from the min/max, use extent legnth for radius
        submesh.bounds.origin = (max_pos + min_pos) / 2.f;
        submesh.bounds.extents = (max_pos - min_pos) / 2.f;
        submesh.bounds.sphere_radius = glm::length(submesh.bounds.extents);

        mesh_data.submeshes.push_back(submesh);
    }

    MemorySource source{Buffer(&mesh_data, sizeof(mesh_data))};
    co_await registry.load_direct(mesh_meta, source);
}

void GltfLoader::load_scenes(SourceMetadata meta, const fastgltf::Asset& asset) const
{
    const auto parent_path = std::filesystem::path(meta.resource_id.string).parent_path();
    auto create_name = [&parent_path](const auto& part, ResourceType type)
    {
        return create_name_relative(parent_path, part, type);
    };

    std::vector<NodeDescription> nodes;
    for (auto& node : asset.nodes)
    {
        NodeDescription node_description{
            .name = STRING_ID(fmt::format("node-{}", node.name))
        };

        if (node.meshIndex.has_value())
        {
            auto& mesh = asset.meshes[node.meshIndex.value()];

            node_description.components.emplace_back(
                MeshSceneComponent{
                    STRING_ID(create_name(mesh.name, ResourceType::Mesh)),
                    mesh.primitives | std::views::transform(
                        [&asset, &create_name](const auto& primitive)
                        {
                            auto& material = asset.materials[primitive.materialIndex.value()];
                            return STRING_ID(create_name(material.name, ResourceType::Material));
                        }
                    ) | std::ranges::to<std::vector>()
                }
            );
        }

        std::visit(
            fastgltf::visitor{
                [&node_description](fastgltf::math::fmat4x4 matrix) mutable
                {
                    TransformSceneComponent transform_component;
                    transform_component.transform = glm::make_mat4(matrix.data());
                    node_description.components.emplace_back(transform_component);
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

                    node_description.components.emplace_back(TransformSceneComponent{.transform = tm * rm * sm});
                }
            },
            node.transform
        );

        nodes.emplace_back(node_description);
    }

    for (size_t i = 0; i < asset.nodes.size(); i++)
    {
        const fastgltf::Node& node = asset.nodes[i];
        auto& scene_node = nodes[i];

        for (auto& c : node.children)
        {
            scene_node.children.push_back(nodes[c].name);
            nodes[c].parent = scene_node.name;
        }
    }

    // Due to design differences between my implementation and glTF's implementation we copy all defines nodes for each scene.
    // In my implementation, each node exists only in one scene (as it does not hold data, only pointers to components),
    // While glTF's allows multiple scenes to hold the same node.

    llvm::SmallVector<Job<>> scene_jobs;
    scene_jobs.reserve(asset.scenes.size());
    auto composite_meta = std::get<CompositeMetadata>(meta.meta);

    for (const auto& [nodeIndices, name] : asset.scenes)
    {
        scene_jobs.emplace_back(
            [&]() -> Job<>
            {
                auto scene_metadata = composite_meta.children.at(create_name(name.c_str(), ResourceType::Scene));

                // TODO: filter nodes per `nodeIndices`

                SceneDescription scene_description;
                scene_description.nodes = nodes;
                scene_description.scene_nodes_ids = std::vector(nodeIndices.begin(), nodeIndices.end());

                std::stringstream ss;
                BinarySerializer serializer{ss};
                serializer.add_value(scene_description);

                auto data = ss.str();
                MemorySource source{Buffer(data.data(), data.size())};
                co_await registry.load_direct(scene_metadata, source);
            }()
        );
    }
    registry.wait_all(scene_jobs);
}
} // portal
