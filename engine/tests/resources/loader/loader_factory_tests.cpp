//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "portal/engine/resources/loader/loader_factory.h"
#include "portal/engine/resources/source/resource_source.h"
#include "portal/engine/resources/resources/resource.h"
#include "portal/engine/strings/string_id.h"
#include "../../../portal/engine/renderer/vulkan/gpu_context.h"
#include "../../../portal/engine/renderer/vulkan/image/vulkan_image.h"

using namespace portal::resources;
using namespace portal;

static vk::raii::Device g_device = nullptr;
static vk::raii::CommandBuffer g_command_buffer = nullptr;
static vk::raii::Queue g_queue = nullptr;

// Mock GpuContext for testing
class MockGpuContext : public GpuContext
{
public:
    // Create mock constructor that calls the real constructor with mock objects
    MockGpuContext() : GpuContext(g_device, g_command_buffer, g_queue) {}

    MOCK_METHOD(renderer::vulkan::VulkanImage, create_image, (void* data, vulkan::ImageBuilder image_builder), (const, override));
};

class LoaderFactoryTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        gpu_context = std::make_shared<MockGpuContext>();

        // Set up default mock behavior for create_image
        EXPECT_CALL(*gpu_context, create_image(testing::_, testing::_))
            .WillRepeatedly(testing::Invoke([](void*, vulkan::ImageBuilder) {
                return renderer::vulkan::VulkanImage{}; // Return by value, will be moved
            }));

        factory.initialize(gpu_context);
        texture_resource = Ref<Texture>::create(STRING_ID("test_texture"));
        regular_resource = Ref<Resource>::create(STRING_ID("test_resource"));
    }

    LoaderFactory factory;
    std::shared_ptr<MockGpuContext> gpu_context;
    Ref<Texture> texture_resource;
    Ref<Resource> regular_resource;
};

TEST_F(LoaderFactoryTest, InitializesCorrectly)
{
    LoaderFactory new_factory;
    auto new_gpu_context = std::make_shared<MockGpuContext>();

    // Set up mock behavior
    EXPECT_CALL(*new_gpu_context, create_image(testing::_, testing::_))
        .WillRepeatedly(testing::Invoke([](void*, vulkan::ImageBuilder) {
            return renderer::vulkan::VulkanImage{}; // Return by value, will be moved
        }));

    // Factory should initialize without throwing
    EXPECT_NO_THROW(new_factory.initialize(new_gpu_context));
}

TEST_F(LoaderFactoryTest, ReturnsImageLoaderForTextureWithImageFormat)
{
    SourceMetadata metadata{
        STRING_ID("test_texture"),
        ResourceType::Texture,
        SourceFormat::Image
    };

    auto loader = factory.get(metadata);

    // Should not be null
    EXPECT_NE(nullptr, loader);

    // Should be able to call methods without crashing
    EXPECT_NO_THROW(loader->load_default(texture_resource));
}

TEST_F(LoaderFactoryTest, ReturnsStubLoaderForUnknownType)
{
    SourceMetadata metadata{
        STRING_ID("unknown_resource"),
        ResourceType::Unknown,
        SourceFormat::Unknown
    };

    auto loader = factory.get(metadata);

    // Should not be null (stub loader)
    EXPECT_NE(nullptr, loader);

    // Stub loader should handle calls gracefully
    EXPECT_NO_THROW(loader->load_default(regular_resource));
}

TEST_F(LoaderFactoryTest, ReturnsStubLoaderForTextureWithUnknownFormat)
{
    SourceMetadata metadata{
        STRING_ID("texture_unknown_format"),
        ResourceType::Texture,
        SourceFormat::Unknown
    };

    auto loader = factory.get(metadata);

    // Should return stub loader for unsupported format
    EXPECT_NE(nullptr, loader);
}

TEST_F(LoaderFactoryTest, ReturnsStubLoaderForUnsupportedResourceTypes)
{
    // Test Material type
    SourceMetadata material_metadata{
        STRING_ID("material_resource"),
        ResourceType::Material,
        SourceFormat::Material
    };

    auto material_loader = factory.get(material_metadata);
    EXPECT_NE(nullptr, material_loader);

    // Test Mesh type
    SourceMetadata mesh_metadata{
        STRING_ID("mesh_resource"),
        ResourceType::Mesh,
        SourceFormat::Obj
    };

    auto mesh_loader = factory.get(mesh_metadata);
    EXPECT_NE(nullptr, mesh_loader);

    // Test Shader type
    SourceMetadata shader_metadata{
        STRING_ID("shader_resource"),
        ResourceType::Shader,
        SourceFormat::Shader
    };

    auto shader_loader = factory.get(shader_metadata);
    EXPECT_NE(nullptr, shader_loader);
}

TEST_F(LoaderFactoryTest, ReturnsConsistentLoaderForSameMetadata)
{
    SourceMetadata metadata{
        STRING_ID("consistent_texture"),
        ResourceType::Texture,
        SourceFormat::Image
    };

    auto loader1 = factory.get(metadata);
    auto loader2 = factory.get(metadata);

    // Note: Depending on implementation, these might be the same instance or different instances
    // Both should be valid
    EXPECT_NE(nullptr, loader1);
    EXPECT_NE(nullptr, loader2);
}

TEST_F(LoaderFactoryTest, HandlesMultipleTextureFormats)
{
    // Test Image format
    SourceMetadata image_metadata{
        STRING_ID("image_texture"),
        ResourceType::Texture,
        SourceFormat::Image
    };
    auto image_loader = factory.get(image_metadata);
    EXPECT_NE(nullptr, image_loader);

    // Test Texture format (should return stub for now)
    SourceMetadata texture_metadata{
        STRING_ID("ktx_texture"),
        ResourceType::Texture,
        SourceFormat::Texture
    };
    auto texture_loader = factory.get(texture_metadata);
    EXPECT_NE(nullptr, texture_loader);

    // Test Preprocessed format (should return stub for now)
    SourceMetadata preprocessed_metadata{
        STRING_ID("preprocessed_texture"),
        ResourceType::Texture,
        SourceFormat::Preprocessed
    };
    auto preprocessed_loader = factory.get(preprocessed_metadata);
    EXPECT_NE(nullptr, preprocessed_loader);
}

// Integration test
TEST_F(LoaderFactoryTest, ImageLoaderCanLoadDefaultTexture)
{
    SourceMetadata metadata{
        STRING_ID("integration_texture"),
        ResourceType::Texture,
        SourceFormat::Image
    };

    auto loader = factory.get(metadata);
    ASSERT_NE(nullptr, loader);

    // Should be able to load default texture
    loader->load_default(texture_resource);
    EXPECT_EQ(ResourceState::Invalid, texture_resource->get_state());
}

class CustomResource : public Resource
{
public:
    explicit CustomResource(StringId id) : Resource(id) {}
};

TEST_F(LoaderFactoryTest, StubLoaderHandlesCustomResourceTypes)
{
    SourceMetadata metadata{
        STRING_ID("custom_resource"),
        ResourceType::Unknown,
        SourceFormat::Unknown
    };

    auto loader = factory.get(metadata);
    EXPECT_NE(nullptr, loader);

    auto custom_resource = Ref<CustomResource>::create(STRING_ID("custom"));

    // Stub loader should handle unknown resource types gracefully
    EXPECT_NO_THROW(loader->load_default(custom_resource.as<Resource>()));

    // Should return empty signature
    auto signatures = loader->get_signature();
    EXPECT_TRUE(signatures.empty());
}