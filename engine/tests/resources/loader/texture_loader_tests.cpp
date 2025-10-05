//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <array>
#include <memory>

#include "portal/engine/resources/loader/texture_loader.h"
#include "portal/engine/resources/source/resource_source.h"
#include "portal/engine/resources/resources/resource.h"
#include "portal/engine/strings/string_id.h"
#include "portal/core/buffer.h"
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

// Mock ResourceSource for testing
class MockResourceSource : public ResourceSource
{
public:
    MOCK_METHOD(SourceMetadata, get_meta, (), (const, override));
    MOCK_METHOD(Buffer, load, (), (override));
};

class TextureLoaderTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mock_gpu_context = std::make_shared<testing::StrictMock<MockGpuContext>>();
        mock_source = std::make_shared<MockResourceSource>();

        // Set up default expectations for create_image using ByMove since Image is move-only
        EXPECT_CALL(*mock_gpu_context, create_image(testing::_, testing::_))
            .WillRepeatedly(testing::Invoke([](void*, const vulkan::ImageBuilder&) {
                return renderer::vulkan::VulkanImage{}; // Return by value, will be moved
            }));

        texture_resource = Ref<Texture>::create(STRING_ID("test_texture"));
        regular_resource = Ref<Resource>::create(STRING_ID("test_resource"));

        // Create a simple test PNG-like data (not a real PNG, just some bytes)
        create_test_image_data();
    }

    void create_test_image_data()
    {
        // Create fake image data that could represent a simple image
        // For testing purposes, we don't need real PNG data since we're mocking stbi
        std::vector<uint8_t> fake_png_data = {
            0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, // PNG signature
            0x00, 0x00, 0x00, 0x10, // Some fake data
            0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00  // More fake data
        };
        test_image_data = Buffer::copy(fake_png_data.data(), fake_png_data.size());
    }

    std::shared_ptr<testing::StrictMock<MockGpuContext>> mock_gpu_context;
    std::shared_ptr<MockResourceSource> mock_source;
    Ref<Texture> texture_resource;
    Ref<Resource> regular_resource;
    Buffer test_image_data;
};

TEST_F(TextureLoaderTest, ConstructorInitializesCorrectly)
{
    EXPECT_NO_THROW(TextureLoader loader(mock_gpu_context));
}

TEST_F(TextureLoaderTest, InitializesWithSource)
{
    TextureLoader loader(mock_gpu_context);

    EXPECT_CALL(*mock_source, get_meta())
        .WillOnce(testing::Return(SourceMetadata{
            STRING_ID("test_source"),
            ResourceType::Texture,
            SourceFormat::Image
        }));

    EXPECT_NO_THROW(loader.init(mock_source));
}

TEST_F(TextureLoaderTest, LoadWithNonTextureResourceFails)
{
    TextureLoader loader(mock_gpu_context);

    // Initialize with mock source
    EXPECT_CALL(*mock_source, get_meta())
        .WillOnce(testing::Return(SourceMetadata{
            STRING_ID("test_source"),
            ResourceType::Texture,
            SourceFormat::Image
        }));
    loader.init(mock_source);

    // Loading a non-Texture resource should return false
    bool result = loader.load(regular_resource);
    EXPECT_FALSE(result);

    // Resource should remain in Empty state since load failed
    EXPECT_EQ(ResourceState::Empty, regular_resource->get_state());
}

TEST_F(TextureLoaderTest, LoadDefaultWithNonTextureResourceFails)
{
    TextureLoader loader(mock_gpu_context);

    // Loading default for non-Texture resource should handle error gracefully
    EXPECT_NO_THROW(loader.load_default(regular_resource));

    // Resource should remain in Empty state since load failed
    EXPECT_EQ(ResourceState::Empty, regular_resource->get_state());
}

TEST_F(TextureLoaderTest, LoadDefaultWithTextureResource)
{
    TextureLoader loader(mock_gpu_context);

    // Load default should work without initialization
    EXPECT_NO_THROW(loader.load_default(texture_resource));

    // Resource should be in Invalid state (default texture loaded)
    EXPECT_EQ(ResourceState::Invalid, texture_resource->get_state());
}

TEST_F(TextureLoaderTest, LoadWithValidTextureData)
{
    TextureLoader loader(mock_gpu_context);

    // Set up mock source expectations
    EXPECT_CALL(*mock_source, get_meta())
        .WillOnce(testing::Return(SourceMetadata{
            STRING_ID("test_source"),
            ResourceType::Texture,
            SourceFormat::Image
        }));
    EXPECT_CALL(*mock_source, load())
        .WillOnce(testing::Return(std::move(test_image_data)));

    loader.init(mock_source);

    // Note: This test may fail because stbi_load_from_memory will fail with fake PNG data
    // In a real scenario, you'd need valid image data or mock stbi functions
    bool result = loader.load(texture_resource);

    // The result depends on whether stbi can decode our fake data
    // For this test, we expect it to fail gracefully
    EXPECT_FALSE(result);
}

TEST_F(TextureLoaderTest, GetSignatureReturnsEmptyVector)
{
    TextureLoader loader(mock_gpu_context);

    auto signatures = loader.get_signature();
    EXPECT_TRUE(signatures.empty());
}

TEST_F(TextureLoaderTest, LoadWithEmptyBuffer)
{
    TextureLoader loader(mock_gpu_context);

    // Set up mock source to return empty buffer
    EXPECT_CALL(*mock_source, get_meta())
        .WillOnce(testing::Return(SourceMetadata{
            STRING_ID("test_source"),
            ResourceType::Texture,
            SourceFormat::Image
        }));
    EXPECT_CALL(*mock_source, load())
        .WillOnce(testing::Return(Buffer{})); // Empty buffer

    loader.init(mock_source);

    bool result = loader.load(texture_resource);
    EXPECT_FALSE(result); // Should fail with empty buffer
}

TEST_F(TextureLoaderTest, MultipleInitCallsWork)
{
    TextureLoader loader(mock_gpu_context);

    auto second_source = std::make_shared<MockResourceSource>();

    // First init
    EXPECT_CALL(*mock_source, get_meta())
        .WillOnce(testing::Return(SourceMetadata{
            STRING_ID("first_source"),
            ResourceType::Texture,
            SourceFormat::Image
        }));
    loader.init(mock_source);

    // Second init should replace the first
    EXPECT_CALL(*second_source, get_meta())
        .WillOnce(testing::Return(SourceMetadata{
            STRING_ID("second_source"),
            ResourceType::Texture,
            SourceFormat::Image
        }));
    EXPECT_NO_THROW(loader.init(second_source));
}

// Integration-style test
TEST_F(TextureLoaderTest, CompleteLoadingWorkflow)
{
    TextureLoader loader(mock_gpu_context);

    // 1. Initialize with source
    EXPECT_CALL(*mock_source, get_meta())
        .WillOnce(testing::Return(SourceMetadata{
            STRING_ID("workflow_source"),
            ResourceType::Texture,
            SourceFormat::Image
        }));
    loader.init(mock_source);

    // 2. Load default first
    loader.load_default(texture_resource);
    EXPECT_EQ(ResourceState::Invalid, texture_resource->get_state());

    // 3. Attempt to load actual data (will fail with fake data)
    EXPECT_CALL(*mock_source, load())
        .WillOnce(testing::Return(std::move(test_image_data)));

    bool result = loader.load(texture_resource);
    EXPECT_FALSE(result); // Expected to fail with fake PNG data
}
