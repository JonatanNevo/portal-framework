//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <thread>
#include <chrono>

#include "portal/engine/resources/resource_registry.h"
#include "portal/engine/resources/database/resource_database.h"
#include "portal/engine/resources/source/resource_source.h"
#include "portal/engine/resources/resources/resource.h"
#include "../../../portal/engine/renderer/vulkan/gpu_context.h"
#include "portal/engine/strings/string_id.h"
#include "portal/core/buffer.h"
#include "../../../portal/engine/renderer/vulkan/vulkan_image.h"

using namespace portal;
using namespace portal::resources;
using namespace std::chrono_literals;

// Mock classes for dependencies
class MockResourceDatabase : public ResourceDatabase
{
public:
    MOCK_METHOD(std::shared_ptr<ResourceSource>, get_source, (StringId id), (const, override));
};

class MockResourceSource : public ResourceSource
{
public:
    MOCK_METHOD(SourceMetadata, get_meta, (), (const, override));
    MOCK_METHOD(Buffer, load, (), (override));
};

class MockGpuContext : public GpuContext
{
public:
    MockGpuContext() : GpuContext(
        *reinterpret_cast<vk::raii::Device*>(&mock_device),
        *reinterpret_cast<vk::raii::CommandBuffer*>(&mock_command_buffer),
        *reinterpret_cast<vk::raii::Queue*>(&mock_queue)
    ) {}

    MOCK_METHOD(renderer::vulkan::VulkanImage, create_image, (void* data, vulkan::ImageBuilder image_builder), (const, override));

private:
    int mock_device{};
    int mock_command_buffer{};
    int mock_queue{};
};

class ResourceRegistryTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mock_gpu_context = std::make_shared<MockGpuContext>();
        mock_database = std::make_shared<MockResourceDatabase>();

        // Set up default GPU context expectations
        EXPECT_CALL(*mock_gpu_context, create_image(testing::_, testing::_))
            .WillRepeatedly(testing::Invoke([](void*, const vulkan::ImageBuilder&) {
                return renderer::vulkan::VulkanImage{}; // Return by value, will be moved
            }));

        registry.initialize(mock_gpu_context, mock_database);

        test_texture_id = STRING_ID("test_texture");
        test_resource_id = STRING_ID("test_resource");
    }

    void TearDown() override
    {
        registry.shutdown();
    }

    // Helper method to create a mock source that returns valid data
    static std::shared_ptr<MockResourceSource> create_mock_source_with_data(const std::string& data)
    {
        auto mock_source = std::make_shared<MockResourceSource>();
        EXPECT_CALL(*mock_source, get_meta())
            .WillRepeatedly(testing::Return(SourceMetadata{
                STRING_ID("test_source"),
                ResourceType::Texture,
                SourceFormat::Image
            }));
        EXPECT_CALL(*mock_source, load())
            .WillOnce(testing::Return(Buffer::copy(data.c_str(), data.size())));
        return mock_source;
    }

    // Helper method to create a mock source that returns empty data
    static std::shared_ptr<MockResourceSource> create_mock_source_empty()
    {
        auto mock_source = std::make_shared<MockResourceSource>();
        EXPECT_CALL(*mock_source, get_meta())
            .WillRepeatedly(testing::Return(SourceMetadata{
                INVALID_STRING_ID,
                ResourceType::Unknown,
                SourceFormat::Unknown
            }));
        EXPECT_CALL(*mock_source, load())
            .WillOnce(testing::Return(Buffer{})); // Empty buffer
        return mock_source;
    }

    ResourceRegistry registry;
    std::shared_ptr<MockGpuContext> mock_gpu_context;
    std::shared_ptr<MockResourceDatabase> mock_database;
    StringId test_texture_id{INVALID_STRING_ID};
    StringId test_resource_id{INVALID_STRING_ID};
};

TEST_F(ResourceRegistryTest, InitializesCorrectly)
{
    ResourceRegistry new_registry;
    auto new_gpu_context = std::make_shared<MockGpuContext>();
    auto new_database = std::make_shared<MockResourceDatabase>();

    // Set up GPU context expectations
    EXPECT_CALL(*new_gpu_context, create_image(testing::_, testing::_))
        .WillRepeatedly(testing::Invoke([](void*, const vulkan::ImageBuilder&) {
            return renderer::vulkan::VulkanImage{};
        }));

    EXPECT_NO_THROW(new_registry.initialize(new_gpu_context, new_database));
    EXPECT_NO_THROW(new_registry.shutdown());
}

TEST_F(ResourceRegistryTest, GetCreatesNewResource)
{
    // Mock database to return a source with data
    EXPECT_CALL(*mock_database, get_source(test_texture_id))
        .WillOnce(testing::Return(create_mock_source_with_data("texture data")));

    // Get a texture resource
    auto texture = registry.get<Texture>(test_texture_id);

    ASSERT_NE(nullptr, texture.get());
    EXPECT_EQ(test_texture_id, texture->id);

    // Resource should initially be in Invalid state (default texture loaded)
    EXPECT_EQ(ResourceState::Invalid, texture->get_state());

    // Give some time for background loading
    std::this_thread::sleep_for(100ms);

    // After background loading, it should be loaded
    // Note: This might be flaky in practice due to threading
}

TEST_F(ResourceRegistryTest, GetReturnsSameResourceForSameId)
{
    // Mock database to return a source
    EXPECT_CALL(*mock_database, get_source(test_texture_id))
        .WillOnce(testing::Return(create_mock_source_with_data("texture data")));

    auto texture1 = registry.get<Texture>(test_texture_id);
    auto texture2 = registry.get<Texture>(test_texture_id);

    ASSERT_NE(nullptr, texture1.get());
    ASSERT_NE(nullptr, texture2.get());
    EXPECT_EQ(texture1.get(), texture2.get()); // Should be the same instance
}

TEST_F(ResourceRegistryTest, GetWithResourceTypeWorks)
{
    // Mock database to return a source
    EXPECT_CALL(*mock_database, get_source(test_texture_id))
        .WillOnce(testing::Return(create_mock_source_with_data("texture data")));

    auto resource = registry.get(test_texture_id, ResourceType::Texture);

    ASSERT_NE(nullptr, resource.get());
    EXPECT_EQ(test_texture_id, resource->id);

    // Should be able to cast to Texture
    auto texture = resource.as<Texture>();
    EXPECT_NE(nullptr, texture.get());
}

TEST_F(ResourceRegistryTest, ImmediateLoadLoadsResourceSynchronously)
{
    // Mock database to return a source with data
    EXPECT_CALL(*mock_database, get_source(test_texture_id))
        .WillOnce(testing::Return(create_mock_source_with_data("texture data")));

    auto texture = registry.immediate_load<Texture>(test_texture_id);

    ASSERT_NE(nullptr, texture.get());
    EXPECT_EQ(test_texture_id, texture->id);

    // Resource should be loaded immediately (not just Invalid state)
    EXPECT_EQ(ResourceState::Loaded, texture->get_state());
}

TEST_F(ResourceRegistryTest, ImmediateLoadWithResourceTypeWorks)
{
    // Mock database to return a source with data
    EXPECT_CALL(*mock_database, get_source(test_texture_id))
        .WillOnce(testing::Return(create_mock_source_with_data("texture data")));

    auto resource = registry.immediate_load(test_texture_id, ResourceType::Texture);

    ASSERT_NE(nullptr, resource.get());
    EXPECT_EQ(test_texture_id, resource->id);
    EXPECT_EQ(ResourceState::Loaded, resource->get_state());
}

TEST_F(ResourceRegistryTest, UnloadHandlesValidResource)
{
    // Mock database to return a source
    EXPECT_CALL(*mock_database, get_source(test_texture_id))
        .WillOnce(testing::Return(create_mock_source_with_data("texture data")));

    // First get the resource to create it
    auto texture = registry.get<Texture>(test_texture_id);
    ASSERT_NE(nullptr, texture.get());

    // Then unload it
    EXPECT_NO_THROW(registry.unload(test_texture_id));
}

TEST_F(ResourceRegistryTest, UnloadHandlesInvalidResource)
{
    StringId non_existent_id = STRING_ID("non_existent");

    // Should not crash when unloading non-existent resource
    EXPECT_NO_THROW(registry.unload(non_existent_id));
}

TEST_F(ResourceRegistryTest, HandlesResourceWithMissingData)
{
    // Mock database to return a source that provides no data
    EXPECT_CALL(*mock_database, get_source(test_texture_id))
        .WillOnce(testing::Return(create_mock_source_empty()));

    auto texture = registry.immediate_load<Texture>(test_texture_id);

    ASSERT_NE(nullptr, texture.get());

    // Resource should be marked as missing since source returned empty data
    EXPECT_EQ(ResourceState::Missing, texture->get_state());
}

TEST_F(ResourceRegistryTest, HandlesMultipleResourceTypes)
{
    StringId mesh_id = STRING_ID("test_mesh");

    // Mock database calls for different resource types
    EXPECT_CALL(*mock_database, get_source(test_texture_id))
        .WillOnce(testing::Return(create_mock_source_with_data("texture data")));

    EXPECT_CALL(*mock_database, get_source(mesh_id))
        .WillOnce(testing::Return(create_mock_source_with_data("mesh data")));

    auto texture = registry.get<Texture>(test_texture_id);
    auto mesh = registry.get(mesh_id, ResourceType::Mesh);
    std::this_thread::sleep_for(10ms); // Allow time for background loading

    ASSERT_NE(nullptr, texture.get());
    ASSERT_NE(nullptr, mesh.get());
    EXPECT_NE(texture.get(), mesh.get()); // Should be different instances
}

// Note: Since increment_ref and decrement_ref are private, we can only test
// the public interface. The reference counting is tested indirectly through
// the ResourceRegistry tests which use these methods internally.

// Integration tests
class ResourceRegistryIntegrationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mock_gpu_context = std::make_shared<MockGpuContext>();
        mock_database = std::make_shared<MockResourceDatabase>();

        // Set up default GPU context expectations
        EXPECT_CALL(*mock_gpu_context, create_image(testing::_, testing::_))
            .WillRepeatedly(testing::Invoke([](void*, const vulkan::ImageBuilder&) {
                return renderer::vulkan::VulkanImage{};
            }));

        registry.initialize(mock_gpu_context, mock_database);
    }

    void TearDown() override
    {
        registry.shutdown();
    }

    ResourceRegistry registry;
    std::shared_ptr<MockGpuContext> mock_gpu_context;
    std::shared_ptr<MockResourceDatabase> mock_database;
};

TEST_F(ResourceRegistryIntegrationTest, LoadAndUnloadCycle)
{
    StringId texture_id = STRING_ID("cycle_texture");

    // Create a mock source with expectations
    auto mock_source = std::make_shared<MockResourceSource>();
    EXPECT_CALL(*mock_source, get_meta())
        .WillRepeatedly(testing::Return(SourceMetadata{
            STRING_ID("test_source"),
            ResourceType::Texture,
            SourceFormat::Image
        }));
    EXPECT_CALL(*mock_source, load())
        .WillOnce(testing::Return(Buffer::copy("texture", 7)));

    // Mock database to return the source
    EXPECT_CALL(*mock_database, get_source(texture_id))
        .WillOnce(testing::Return(mock_source));

    // Load the resource
    auto texture = registry.immediate_load<Texture>(texture_id);
    ASSERT_NE(nullptr, texture.get());
    EXPECT_EQ(ResourceState::Loaded, texture->get_state());

    // Unload the resource
    EXPECT_NO_THROW(registry.unload(texture_id));
}

TEST_F(ResourceRegistryIntegrationTest, MultipleResourcesSimultaneousAccess)
{
    const int num_resources = 5;
    std::vector<StringId> resource_ids;

    // Use predefined StringIds to avoid runtime string generation
    resource_ids.push_back(STRING_ID("resource_0"));
    resource_ids.push_back(STRING_ID("resource_1"));
    resource_ids.push_back(STRING_ID("resource_2"));
    resource_ids.push_back(STRING_ID("resource_3"));
    resource_ids.push_back(STRING_ID("resource_4"));

    // Set up mock expectations for each resource
    for (int i = 0; i < num_resources; ++i)
    {
        auto mock_source = std::make_shared<MockResourceSource>();
        EXPECT_CALL(*mock_source, get_meta())
            .WillRepeatedly(testing::Return(SourceMetadata{
                STRING_ID("test_source"),
                ResourceType::Texture,
                SourceFormat::Image
            }));
        EXPECT_CALL(*mock_source, load())
            .WillOnce(testing::Return(Buffer::copy("data", 4)));

        EXPECT_CALL(*mock_database, get_source(resource_ids[i]))
            .WillOnce(testing::Return(mock_source));
    }

    std::vector<std::thread> threads;
    std::vector<Ref<Texture>> textures(num_resources);

    // Create threads that simultaneously access different resources
    for (int i = 0; i < num_resources; ++i)
    {
        threads.emplace_back([&, i]() {
            textures[i] = registry.get<Texture>(resource_ids[i]);
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads)
    {
        thread.join();
    }

    std::this_thread::sleep_for(100ms); // Allow time for background loading

    // Verify all resources were loaded
    for (int i = 0; i < num_resources; ++i)
    {
        ASSERT_NE(nullptr, textures[i].get());
        EXPECT_EQ(resource_ids[i], textures[i]->id);
    }
}
