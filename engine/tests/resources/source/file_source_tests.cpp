//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>

#include "portal/engine/resources/source/file_source.h"
#include "portal/engine/resources/resource_types.h"
#include "portal/core/buffer.h"

using namespace portal::resources;
using namespace portal;

class FileSourceTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create temporary test directory
        test_dir = std::filesystem::temp_directory_path() / "portal_file_source_tests";
        std::filesystem::create_directories(test_dir);

        // Create test files with different extensions
        create_test_file("test_texture.png", "fake png data");
        create_test_file("test_mesh.obj", "fake obj data");
        create_test_file("test_material.mtl", "fake mtl data");
        create_test_file("test_shader.slang", "fake slang data");
        create_test_file("test_composite.glb", "fake glb data");
        create_test_file("test_unknown.xyz", "unknown file type");
    }

    void TearDown() override
    {
        // Clean up test files
        std::filesystem::remove_all(test_dir);
    }

    void create_test_file(const std::string& filename, const std::string& content)
    {
        auto file_path = test_dir / filename;
        std::ofstream file(file_path);
        file << content;
    }

    std::filesystem::path test_dir;
};

// Test resource type detection for various file extensions
TEST_F(FileSourceTest, DetectsTextureType)
{
    FileSource png_source(test_dir / "test_texture.png");
    auto meta = png_source.get_meta();
    EXPECT_EQ(ResourceType::Texture, meta.resource_type);
    EXPECT_EQ(SourceFormat::Image, meta.format);

    // Test other texture formats
    create_test_file("test.jpg", "fake jpg");
    FileSource jpg_source(test_dir / "test.jpg");
    meta = jpg_source.get_meta();
    EXPECT_EQ(ResourceType::Texture, meta.resource_type);
    EXPECT_EQ(SourceFormat::Image, meta.format);

    create_test_file("test.jpeg", "fake jpeg");
    FileSource jpeg_source(test_dir / "test.jpeg");
    meta = jpeg_source.get_meta();
    EXPECT_EQ(ResourceType::Texture, meta.resource_type);
    EXPECT_EQ(SourceFormat::Image, meta.format);

    create_test_file("test.hdr", "fake hdr");
    FileSource hdr_source(test_dir / "test.hdr");
    meta = hdr_source.get_meta();
    EXPECT_EQ(ResourceType::Texture, meta.resource_type);
    EXPECT_EQ(SourceFormat::Image, meta.format);
}

TEST_F(FileSourceTest, DetectsMeshType)
{
    FileSource source(test_dir / "test_mesh.obj");
    auto meta = source.get_meta();
    EXPECT_EQ(ResourceType::Mesh, meta.resource_type);
    EXPECT_EQ(SourceFormat::Obj, meta.format);
}

TEST_F(FileSourceTest, DetectsMaterialType)
{
    FileSource source(test_dir / "test_material.mtl");
    auto meta = source.get_meta();
    EXPECT_EQ(ResourceType::Material, meta.resource_type);
    EXPECT_EQ(SourceFormat::Material, meta.format);
}

TEST_F(FileSourceTest, DetectsShaderType)
{
    FileSource source(test_dir / "test_shader.slang");
    auto meta = source.get_meta();
    EXPECT_EQ(ResourceType::Shader, meta.resource_type);
    EXPECT_EQ(SourceFormat::Shader, meta.format);
}

TEST_F(FileSourceTest, DetectsCompositeType)
{
    FileSource glb_source(test_dir / "test_composite.glb");
    auto meta = glb_source.get_meta();
    EXPECT_EQ(ResourceType::Composite, meta.resource_type);
    EXPECT_EQ(SourceFormat::Glft, meta.format);

    // Test GLTF format too
    create_test_file("test.gltf", "fake gltf");
    FileSource gltf_source(test_dir / "test.gltf");
    meta = gltf_source.get_meta();
    EXPECT_EQ(ResourceType::Composite, meta.resource_type);
    EXPECT_EQ(SourceFormat::Glft, meta.format);
}

TEST_F(FileSourceTest, DetectsUnknownType)
{
    FileSource source(test_dir / "test_unknown.xyz");
    auto meta = source.get_meta();
    EXPECT_EQ(ResourceType::Unknown, meta.resource_type);
    EXPECT_EQ(SourceFormat::Unknown, meta.format);
}

TEST_F(FileSourceTest, HandlesNonExistentFile)
{
    FileSource source(test_dir / "nonexistent.png");
    auto meta = source.get_meta();
    EXPECT_EQ(ResourceType::Unknown, meta.resource_type);
    EXPECT_EQ(SourceFormat::Unknown, meta.format);
}

TEST_F(FileSourceTest, LoadsFileContent)
{
    const std::string test_content = "test file content for loading";
    create_test_file("loadtest.txt", test_content);

    FileSource source(test_dir / "loadtest.txt");
    Buffer buffer = source.load();

    ASSERT_NE(nullptr, buffer.data);
    EXPECT_EQ(test_content.size(), buffer.size);

    // Compare content
    std::string loaded_content(static_cast<const char*>(buffer.data), buffer.size);
    EXPECT_EQ(test_content, loaded_content);
}

TEST_F(FileSourceTest, HandlesEmptyFile)
{
    create_test_file("empty.txt", "");

    FileSource source(test_dir / "empty.txt");
    Buffer buffer = source.load();

    // Empty file should still return a valid buffer with size 0
    EXPECT_EQ(0, buffer.size);
}

TEST_F(FileSourceTest, HandlesNonExistentFileLoad)
{
    FileSource source(test_dir / "nonexistent.txt");
    Buffer buffer = source.load();

    // Should return empty buffer for non-existent file
    EXPECT_EQ(nullptr, buffer.data);
    EXPECT_EQ(0, buffer.size);
}

TEST_F(FileSourceTest, HandlesLargeFile)
{
    // Create a larger test file
    const size_t large_size = 10000;
    std::string large_content(large_size, 'A');
    create_test_file("large.txt", large_content);

    FileSource source(test_dir / "large.txt");
    Buffer buffer = source.load();

    ASSERT_NE(nullptr, buffer.data);
    EXPECT_EQ(large_size, buffer.size);

    // Verify first and last characters
    const char* content = static_cast<const char*>(buffer.data);
    EXPECT_EQ('A', content[0]);
    EXPECT_EQ('A', content[large_size - 1]);
}

TEST_F(FileSourceTest, HandlesPathWithSpaces)
{
    create_test_file("file with spaces.png", "content");

    FileSource source(test_dir / "file with spaces.png");
    auto meta = source.get_meta();
    EXPECT_EQ(ResourceType::Texture, meta.resource_type);
    EXPECT_EQ(SourceFormat::Image, meta.format);

    Buffer buffer = source.load();
    EXPECT_NE(nullptr, buffer.data);
    EXPECT_GT(buffer.size, 0);
}

// Test polymorphism through base class
TEST_F(FileSourceTest, WorksThroughBaseClass)
{
    const std::unique_ptr<ResourceSource> source = std::make_unique<FileSource>(test_dir / "test_texture.png");

    auto meta = source->get_meta();
    EXPECT_EQ(ResourceType::Texture, meta.resource_type);
    EXPECT_EQ(SourceFormat::Image, meta.format);

    Buffer buffer = source->load();
    EXPECT_NE(nullptr, buffer.data);
    EXPECT_GT(buffer.size, 0);
}

TEST_F(FileSourceTest, ChecksSourceIdFromFilename)
{
    FileSource source(test_dir / "test_texture.png");
    auto meta = source.get_meta();

    // Source ID should be based on the filename
    EXPECT_EQ(STRING_ID("test_texture.png"), meta.source_id);
}

TEST_F(FileSourceTest, HandlesPreprocessedShaderFormat)
{
    create_test_file("shader.spv", "fake spirv data");

    FileSource source(test_dir / "shader.spv");
    auto meta = source.get_meta();
    EXPECT_EQ(ResourceType::Shader, meta.resource_type);
    EXPECT_EQ(SourceFormat::Preprocessed, meta.format);
}
