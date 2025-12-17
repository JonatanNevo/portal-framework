//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <numeric>
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <unordered_map>

#include <glm/glm.hpp>

#include "portal/core/buffer.h"
#include "portal/serialization/archive.h"

using namespace portal;

struct TestObject
{
    int value;
    std::string name;

    void archive(ArchiveObject& archive) const
    {
        archive.add_property("value", value);
        archive.add_property("name", name);
    }

    static TestObject dearchive(ArchiveObject& archive)
    {
        TestObject obj;
        archive.get_property("value", obj.value);
        archive.get_property("name", obj.name);
        return obj;
    }
};

class ArchiveObjectTest : public ::testing::Test
{
protected:
    ArchiveObject archive;
};

// 1. Basic Property Operations
TEST_F(ArchiveObjectTest, AddAndGetBoolProperty)
{
    constexpr bool value = true;
    archive.add_property("test_bool", value);

    bool retrieved;
    ASSERT_TRUE(archive.get_property("test_bool", retrieved));
    EXPECT_EQ(retrieved, value);
}


TEST_F(ArchiveObjectTest, AddAndGetIntegerProperty)
{
    constexpr int value = 42;
    archive.add_property("test_int", value);

    int retrieved;
    ASSERT_TRUE(archive.get_property("test_int", retrieved));
    EXPECT_EQ(retrieved, value);
}

TEST_F(ArchiveObjectTest, AddAndGetFloatProperty)
{
    constexpr float value = 3.14f;
    archive.add_property("test_float", value);

    float retrieved;
    ASSERT_TRUE(archive.get_property("test_float", retrieved));
    EXPECT_FLOAT_EQ(retrieved, value);
}

TEST_F(ArchiveObjectTest, AddAndGetDoubleProperty)
{
    constexpr double value = 2.71828;
    archive.add_property("test_double", value);

    double retrieved;
    ASSERT_TRUE(archive.get_property("test_double", retrieved));
    EXPECT_DOUBLE_EQ(retrieved, value);
}

TEST_F(ArchiveObjectTest, AddAndGetUint128Property)
{
    constexpr uint128_t value = 12345678901234567890ULL;
    archive.add_property("test_uint128", value);

    uint128_t retrieved;
    ASSERT_TRUE(archive.get_property("test_uint128", retrieved));
    EXPECT_EQ(retrieved, value);
}

// 2. Container Types
TEST_F(ArchiveObjectTest, AddAndGetVectorProperty)
{
    const std::vector<int> int_values = {1, 2, 3, 4, 5};
    const std::vector<float> float_values = {1.1, 2.2, 3.3, 4.4, 5.5};
    const std::vector<std::string> string_values = {"first", "second", "third"};
    archive.add_property("int_vector", int_values);
    archive.add_property("float_vector", float_values);
    archive.add_property("string_vector", string_values);

    std::vector<int> retrieved_int;
    std::vector<float> retrieved_float;
    std::vector<std::string> retrieved_string;
    ASSERT_TRUE(archive.get_property("int_vector", retrieved_int));
    ASSERT_TRUE(archive.get_property("float_vector", retrieved_float));
    ASSERT_TRUE(archive.get_property("string_vector", retrieved_string));
    EXPECT_EQ(retrieved_int, int_values);
    EXPECT_EQ(retrieved_float, float_values);
    EXPECT_EQ(retrieved_string, string_values);
}

TEST_F(ArchiveObjectTest, StringsCopyData)
{
    {
        std::vector<std::string> value = {"first", "second", "third"};
        archive.add_property("vector", value);
        value.clear();
        value.emplace_back("something");
        value.emplace_back("else");
    }

    std::vector<std::string> retrieved;
    ASSERT_TRUE(archive.get_property("vector", retrieved));
    std::vector<std::string> expected = {"first", "second", "third"};
    EXPECT_EQ(retrieved, expected);
}

TEST_F(ArchiveObjectTest, AddAndGetStringProperty)
{
    const std::string value = "Hello, World!";
    archive.add_property("test_string", value);

    std::string retrieved;
    ASSERT_TRUE(archive.get_property("test_string", retrieved));
    EXPECT_EQ(retrieved, value);
}

TEST_F(ArchiveObjectTest, AddAndGetGlmVec2Property)
{
    glm::vec2 value(1.0f, 2.0f);
    archive.add_property("test_vec2", value);

    glm::vec2 retrieved;
    ASSERT_TRUE(archive.get_property("test_vec2", retrieved));
    EXPECT_EQ(retrieved.x, value.x);
    EXPECT_EQ(retrieved.y, value.y);
}

TEST_F(ArchiveObjectTest, AddAndGetGlmVec3Property)
{
    glm::vec3 value(1.0f, 2.0f, 3.0f);
    archive.add_property("test_vec3", value);

    glm::vec3 retrieved;
    ASSERT_TRUE(archive.get_property("test_vec3", retrieved));
    EXPECT_EQ(retrieved.x, value.x);
    EXPECT_EQ(retrieved.y, value.y);
    EXPECT_EQ(retrieved.z, value.z);
}

TEST_F(ArchiveObjectTest, AddAndGetGlmVec4Property)
{
    glm::vec4 value(1.0f, 2.0f, 3.0f, 4.0f);
    archive.add_property("test_vec4", value);

    glm::vec4 retrieved;
    ASSERT_TRUE(archive.get_property("test_vec4", retrieved));
    EXPECT_EQ(retrieved.x, value.x);
    EXPECT_EQ(retrieved.y, value.y);
    EXPECT_EQ(retrieved.z, value.z);
    EXPECT_EQ(retrieved.w, value.w);
}

TEST_F(ArchiveObjectTest, AddAndGetMapProperty)
{
    const std::unordered_map<std::string, int> values = {
        {"key1", 10},
        {"key2", 20},
        {"key3", 30}
    };
    archive.add_property("test_map", values);

    std::unordered_map<std::string, int> retrieved;
    ASSERT_TRUE(archive.get_property("test_map", retrieved));
    EXPECT_EQ(retrieved, values);
}

// 3. Binary Data
TEST_F(ArchiveObjectTest, AddAndGetBinaryBlockWithBuffer)
{
    const std::vector<std::byte> data = {(std::byte)0x01, (std::byte)0x02, (std::byte)0x03, (std::byte)0x04, (std::byte)0x05};
    Buffer buffer = Buffer::allocate(data.size());
    buffer.write(data.data(), data.size());

    archive.add_binary_block("binary_data", buffer);

    Buffer retrieved;
    ASSERT_TRUE(archive.get_binary_block("binary_data", retrieved));
    EXPECT_EQ(retrieved.size, buffer.size);
    EXPECT_EQ(std::memcmp(retrieved.data, buffer.data, buffer.size), 0);
}

TEST_F(ArchiveObjectTest, AddAndGetBinaryBlockWithVector)
{
    std::vector<std::byte> data = {(std::byte)0xAA, (std::byte)0xBB, (std::byte)0xCC, (std::byte)0xDD};
    archive.add_binary_block("binary_vector", data);

    std::vector<std::byte> retrieved;
    ASSERT_TRUE(archive.get_binary_block("binary_vector", retrieved));
    EXPECT_EQ(retrieved, data);
}

TEST_F(ArchiveObjectTest, BinaryDataIntegrity)
{
    std::vector<std::byte> original_data(1024);
    for (size_t i = 0; i < original_data.size(); ++i)
    {
        original_data[i] = static_cast<std::byte>(i % 256);
    }
    archive.add_binary_block("large_binary", original_data);

    std::vector<std::byte> retrieved_data;
    ASSERT_TRUE(archive.get_binary_block("large_binary", retrieved_data));
    EXPECT_EQ(retrieved_data, original_data);
}

// 4. Archiveable Objects
TEST_F(ArchiveObjectTest, AddAndGetArchiveableObject)
{
    const TestObject obj{42, "test_object"};
    archive.add_property("test_obj", obj);

    TestObject retrieved;
    ASSERT_TRUE(archive.get_property("test_obj", retrieved));
    EXPECT_EQ(retrieved.value, obj.value);
    EXPECT_EQ(retrieved.name, obj.name);
}

TEST_F(ArchiveObjectTest, NestedObjectSerialization)
{
    const TestObject first_object{100, "first"};
    const TestObject second_object{200, "second"};

    const std::vector vec_objects = {first_object, second_object};
    archive.add_property("nested_objects_vec", vec_objects);

    std::vector<TestObject> retrieved_vec;
    ASSERT_TRUE(archive.get_property("nested_objects_vec", retrieved_vec));
    EXPECT_EQ(retrieved_vec.size(), vec_objects.size());
    for (size_t i = 0; i < vec_objects.size(); ++i)
    {
        EXPECT_EQ(retrieved_vec[i].value, vec_objects[i].value);
        EXPECT_EQ(retrieved_vec[i].name, vec_objects[i].name);
    }


    const std::unordered_map<std::string, TestObject> objects = {{"first", first_object}, {"second", second_object}};
    archive.add_property("nested_objects", objects);

    std::unordered_map<std::string, TestObject> retrieved;
    ASSERT_TRUE(archive.get_property("nested_objects", retrieved));
    EXPECT_EQ(retrieved.size(), objects.size());
    for (const auto& [key, value] : objects)
    {
        auto it = retrieved.find(key);
        ASSERT_NE(it, retrieved.end());
        EXPECT_EQ(it->second.value, value.value);
        EXPECT_EQ(it->second.name, value.name);
    }
}

// 5. Edge Cases
TEST_F(ArchiveObjectTest, EmptyVector)
{
    const std::vector<int> empty_vector;
    archive.add_property("empty_vector", empty_vector);

    std::vector<int> retrieved;
    ASSERT_TRUE(archive.get_property("empty_vector", retrieved));
    EXPECT_TRUE(retrieved.empty());
}

TEST_F(ArchiveObjectTest, EmptyString)
{
    const std::string empty_string;
    archive.add_property("empty_string", empty_string);

    std::string retrieved;
    ASSERT_TRUE(archive.get_property("empty_string", retrieved));
    EXPECT_TRUE(retrieved.empty());
}

TEST_F(ArchiveObjectTest, EmptyMap)
{
    const std::unordered_map<std::string, int> empty_map;
    archive.add_property("empty_map", empty_map);

    std::unordered_map<std::string, int> retrieved;
    ASSERT_TRUE(archive.get_property("empty_map", retrieved));
    EXPECT_TRUE(retrieved.empty());
}

TEST_F(ArchiveObjectTest, LargeDataSet)
{
    std::vector<int> large_vector(10000);
    std::iota(large_vector.begin(), large_vector.end(), 0);

    archive.add_property("large_vector", large_vector);

    std::vector<int> retrieved;
    ASSERT_TRUE(archive.get_property("large_vector", retrieved));
    EXPECT_EQ(retrieved.size(), large_vector.size());
    EXPECT_EQ(retrieved, large_vector);
}

TEST_F(ArchiveObjectTest, PropertyNameConflicts)
{
    archive.add_property("duplicate", 10);
    archive.add_property("duplicate", 20); // Should overwrite

    int retrieved;
    ASSERT_TRUE(archive.get_property("duplicate", retrieved));
    EXPECT_EQ(retrieved, 20);
}

// 7. Error Handling
TEST_F(ArchiveObjectTest, InvalidPropertyAccess)
{
    int value;
    EXPECT_FALSE(archive.get_property("non_existent_property", value));
}

TEST_F(ArchiveObjectTest, NullTerminatedStringHandling)
{
    std::string test_string = "test\0embedded\0nulls";
    archive.add_property("null_string", test_string);

    std::string retrieved;
    ASSERT_TRUE(archive.get_property("null_string", retrieved));
    // Should handle null-terminated strings properly
    EXPECT_EQ(retrieved.length(), test_string.length());
}

// Additional comprehensive tests
TEST_F(ArchiveObjectTest, MultiplePropertyTypes)
{
    archive.add_property("int_prop", 42);
    archive.add_property("float_prop", 3.14f);
    archive.add_property("string_prop", std::string("hello"));
    archive.add_property("vector_prop", std::vector<int>{1, 2, 3});

    int int_val;
    float float_val;
    std::string string_val;
    std::vector<int> vector_val;

    ASSERT_TRUE(archive.get_property("int_prop", int_val));
    ASSERT_TRUE(archive.get_property("float_prop", float_val));
    ASSERT_TRUE(archive.get_property("string_prop", string_val));
    ASSERT_TRUE(archive.get_property("vector_prop", vector_val));

    EXPECT_EQ(int_val, 42);
    EXPECT_FLOAT_EQ(float_val, 3.14f);
    EXPECT_EQ(string_val, "hello");
    EXPECT_EQ(vector_val, (std::vector<int>{1, 2, 3}));
}
