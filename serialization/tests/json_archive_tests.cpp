//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <gtest/gtest.h>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>

#include "portal/serialization/archive/json_archive.h"
#include "portal/core/files/file_system.h"

using namespace portal;

struct TestArchiveObject
{
    int value;
    std::string name;
    std::vector<float> data;

    void archive(ArchiveObject& archive) const
    {
        archive.add_property("value", value);
        archive.add_property("name", name);
        archive.add_property("data", data);
    }

    static TestArchiveObject dearchive(ArchiveObject& archive)
    {
        TestArchiveObject obj;
        archive.get_property("value", obj.value);
        archive.get_property("name", obj.name);
        archive.get_property("data", obj.data);
        return obj;
    }

    bool operator==(const TestArchiveObject& other) const
    {
        return value == other.value && name == other.name && data == other.data;
    }
};

class JsonArchiveTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test_dir = std::filesystem::temp_directory_path() / "json_archive_tests";
        std::filesystem::create_directories(test_dir);
    }

    void TearDown() override
    {
        if (std::filesystem::exists(test_dir))
        {
            std::filesystem::remove_all(test_dir);
        }
    }

    std::filesystem::path test_dir;
    JsonArchive archive;
};

// Basic Property Serialization Tests
TEST_F(JsonArchiveTest, SerializeBasicTypes)
{
    archive.add_property("int_value", 42);
    archive.add_property("float_value", 3.14f);
    archive.add_property("double_value", 2.71828);
    archive.add_property("bool_value", true);
    archive.add_property("string_value", std::string("hello world"));

    std::stringstream ss;
    archive.dump(ss);

    std::string json_output = ss.str();
    EXPECT_FALSE(json_output.empty());

    // Verify JSON contains expected values
    EXPECT_NE(json_output.find("\"int_value\":42"), std::string::npos);
    EXPECT_NE(json_output.find("\"float_value\":3.14"), std::string::npos);
    EXPECT_NE(json_output.find("\"bool_value\":true"), std::string::npos);
    EXPECT_NE(json_output.find("\"string_value\":\"hello world\""), std::string::npos);
}

TEST_F(JsonArchiveTest, SerializeArrayTypes)
{
    const std::vector<int> int_array = {1, 2, 3, 4, 5};
    const std::vector<float> float_array = {1.1f, 2.2f, 3.3f};
    const std::vector<std::string> string_array = {"first", "second", "third"};

    archive.add_property("int_array", int_array);
    archive.add_property("float_array", float_array);
    archive.add_property("string_array", string_array);

    std::stringstream ss;
    archive.dump(ss);

    std::string json_output = ss.str();
    EXPECT_NE(json_output.find("[1,2,3,4,5]"), std::string::npos);
    EXPECT_NE(json_output.find("\"string_array\":[\"first\",\"second\",\"third\"]"), std::string::npos);
}

TEST_F(JsonArchiveTest, SerializeNestedObjects)
{
    TestArchiveObject obj1{100, "object1", {1.0f, 2.0f}};
    TestArchiveObject obj2{200, "object2", {3.0f, 4.0f}};

    archive.add_property("test_object", obj1);

    std::vector<TestArchiveObject> object_array = {obj1, obj2};
    archive.add_property("object_array", object_array);

    std::stringstream ss;
    archive.dump(ss);

    std::string json_output = ss.str();
    EXPECT_NE(json_output.find("\"test_object\""), std::string::npos);
    EXPECT_NE(json_output.find("\"object_array\""), std::string::npos);
    EXPECT_NE(json_output.find("\"object1\""), std::string::npos);
    EXPECT_NE(json_output.find("\"object2\""), std::string::npos);
}

// File I/O Tests
TEST_F(JsonArchiveTest, WriteToFile)
{
    archive.add_property("test_value", 42);
    archive.add_property("test_string", std::string("file test"));

    auto output_path = test_dir / "test_output.json";
    archive.dump(output_path);

    EXPECT_TRUE(std::filesystem::exists(output_path));

    std::ifstream file(output_path);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    EXPECT_NE(content.find("\"test_value\":42"), std::string::npos);
    EXPECT_NE(content.find("\"test_string\":\"file test\""), std::string::npos);
}

TEST_F(JsonArchiveTest, ReadFromFile)
{
    // First write a JSON file
    std::string json_content = R"({
        "int_prop": 123,
        "string_prop": "test string",
        "array_prop": [1, 2, 3],
        "bool_prop": true,
        "float_prop": 3.14
    })";

    auto input_path = test_dir / "test_input.json";
    std::ofstream file(input_path);
    file << json_content;
    file.close();

    // Now read it back
    JsonArchive read_archive;
    read_archive.read(input_path);

    int int_val;
    std::string string_val;
    std::vector<int> array_val;
    bool bool_val;
    float float_val;

    ASSERT_TRUE(read_archive.get_property("int_prop", int_val));
    ASSERT_TRUE(read_archive.get_property("string_prop", string_val));
    ASSERT_TRUE(read_archive.get_property("array_prop", array_val));
    ASSERT_TRUE(read_archive.get_property("bool_prop", bool_val));
    ASSERT_TRUE(read_archive.get_property("float_prop", float_val));

    EXPECT_EQ(int_val, 123);
    EXPECT_EQ(string_val, "test string");
    EXPECT_EQ(array_val, (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(bool_val, true);
    EXPECT_FLOAT_EQ(float_val, 3.14f);
}

TEST_F(JsonArchiveTest, ReadFromStream)
{
    const std::string json_content = R"({
        "nested": {
            "value": 42,
            "name": "nested_object"
        },
        "simple": "value"
    })";

    std::stringstream ss(json_content);
    JsonArchive read_archive;
    read_archive.read(ss);

    std::string simple_val;
    TestArchiveObject nested_obj;
    ASSERT_TRUE(read_archive.get_property("nested", nested_obj));
    ASSERT_TRUE(read_archive.get_property("simple", simple_val));
    EXPECT_EQ(simple_val, "value");
    EXPECT_EQ(nested_obj.value, 42);
    EXPECT_EQ(nested_obj.name, "nested_object");
}


// Round-trip Tests
TEST_F(JsonArchiveTest, RoundTripBasicTypes)
{
    // Write data
    JsonArchive write_archive;
    write_archive.add_property("int_val", 42);
    write_archive.add_property("float_val", 3.14f);
    write_archive.add_property("string_val", std::string("round trip"));
    write_archive.add_property("bool_val", true);

    std::stringstream ss;
    write_archive.dump(ss);

    // Read it back
    JsonArchive read_archive;
    read_archive.read(ss);

    int int_val;
    float float_val;
    std::string string_val;
    bool bool_val;

    ASSERT_TRUE(read_archive.get_property("int_val", int_val));
    ASSERT_TRUE(read_archive.get_property("float_val", float_val));
    ASSERT_TRUE(read_archive.get_property("string_val", string_val));
    ASSERT_TRUE(read_archive.get_property("bool_val", bool_val));

    EXPECT_EQ(int_val, 42);
    EXPECT_FLOAT_EQ(float_val, 3.14f);
    EXPECT_EQ(string_val, "round trip");
    EXPECT_EQ(bool_val, true);
}

TEST_F(JsonArchiveTest, RoundTripArrays)
{
    JsonArchive write_archive;
    std::vector<int> int_array = {1, 2, 3, 4, 5};
    std::vector<std::string> string_array = {"a", "b", "c"};

    write_archive.add_property("int_array", int_array);
    write_archive.add_property("string_array", string_array);

    std::stringstream ss;
    write_archive.dump(ss);

    JsonArchive read_archive;
    read_archive.read(ss);

    std::vector<int> read_int_array;
    std::vector<std::string> read_string_array;

    ASSERT_TRUE(read_archive.get_property("int_array", read_int_array));
    ASSERT_TRUE(read_archive.get_property("string_array", read_string_array));

    EXPECT_EQ(read_int_array, int_array);
    EXPECT_EQ(read_string_array, string_array);
}

TEST_F(JsonArchiveTest, RoundTripComplexObjects)
{
    TestArchiveObject original{42, "test object", {1.0f, 2.0f, 3.0f}};

    JsonArchive write_archive;
    write_archive.add_property("test_obj", original);

    std::stringstream ss;
    write_archive.dump(ss);

    JsonArchive read_archive;
    read_archive.read(ss);

    TestArchiveObject read_obj;
    ASSERT_TRUE(read_archive.get_property("test_obj", read_obj));

    EXPECT_EQ(read_obj, original);
}

// Edge Cases and Error Handling
TEST_F(JsonArchiveTest, EmptyArchive)
{
    std::stringstream ss;
    archive.dump(ss);

    const std::string json_output = ss.str();
    EXPECT_EQ(json_output, "{}");
}

TEST_F(JsonArchiveTest, EmptyContainers)
{
    std::vector<int> empty_vector;
    std::string empty_string;
    std::unordered_map<std::string, int> empty_map;

    archive.add_property("empty_vector", empty_vector);
    archive.add_property("empty_string", empty_string);
    archive.add_property("empty_map", empty_map);

    std::stringstream ss;
    archive.dump(ss);

    JsonArchive read_archive;
    read_archive.read(ss);

    std::vector<int> read_vector;
    std::string read_string;
    std::unordered_map<std::string, int> read_map;

    ASSERT_TRUE(read_archive.get_property("empty_vector", read_vector));
    ASSERT_TRUE(read_archive.get_property("empty_string", read_string));
    ASSERT_TRUE(read_archive.get_property("empty_map", read_map));

    EXPECT_TRUE(read_vector.empty());
    EXPECT_TRUE(read_string.empty());
    EXPECT_TRUE(read_map.empty());
}

TEST_F(JsonArchiveTest, NonExistentFile)
{
    const auto non_existent_path = test_dir / "non_existent.json";

    // Should not crash, but should log error
    archive.read(non_existent_path);

    // Archive should remain empty
    int dummy;
    EXPECT_FALSE(archive.get_property("any_prop", dummy));
}

TEST_F(JsonArchiveTest, InvalidJsonInput)
{
    const std::string invalid_json = "{ invalid json content }";
    std::stringstream ss(invalid_json);

    JsonArchive read_archive;
    // Should not crash but should handle gracefully
    read_archive.read(ss);

    int dummy;
    EXPECT_FALSE(read_archive.get_property("any_prop", dummy));
}

TEST_F(JsonArchiveTest, LargeDataSet)
{
    std::vector<int> large_array(10000);
    std::iota(large_array.begin(), large_array.end(), 0);

    archive.add_property("large_array", large_array);

    std::stringstream ss;
    archive.dump(ss);

    JsonArchive read_archive;
    read_archive.read(ss);

    std::vector<int> read_array;
    ASSERT_TRUE(read_archive.get_property("large_array", read_array));
    EXPECT_EQ(read_array, large_array);
}

TEST_F(JsonArchiveTest, SpecialCharactersInStrings)
{
    const std::string special_string = "Special chars: \n\t\r\"\\";
    archive.add_property("special", special_string);

    std::stringstream ss;
    archive.dump(ss);

    JsonArchive read_archive;
    read_archive.read(ss);

    std::string read_string;
    ASSERT_TRUE(read_archive.get_property("special", read_string));
    EXPECT_EQ(read_string, special_string);
}

TEST_F(JsonArchiveTest, UnicodeStrings)
{
    const std::string unicode_string = "Unicode: 你好 🌍 αβγ שלום עולם";
    archive.add_property("unicode", unicode_string);

    std::stringstream ss;
    archive.dump(ss);

    JsonArchive read_archive;
    read_archive.read(ss);

    std::string read_string;
    ASSERT_TRUE(read_archive.get_property("unicode", read_string));
    EXPECT_EQ(read_string, unicode_string);
}

TEST_F(JsonArchiveTest, NumericalLimits)
{
    archive.add_property("max_int", std::numeric_limits<int>::max());
    archive.add_property("min_int", std::numeric_limits<int>::min());
    archive.add_property("max_float", std::numeric_limits<float>::max());
    archive.add_property("min_float", std::numeric_limits<float>::lowest());

    std::stringstream ss;
    archive.dump(ss);

    JsonArchive read_archive;
    read_archive.read(ss);

    int max_int, min_int;
    float max_float, min_float;

    ASSERT_TRUE(read_archive.get_property("max_int", max_int));
    ASSERT_TRUE(read_archive.get_property("min_int", min_int));
    ASSERT_TRUE(read_archive.get_property("max_float", max_float));
    ASSERT_TRUE(read_archive.get_property("min_float", min_float));

    EXPECT_EQ(max_int, std::numeric_limits<int>::max());
    EXPECT_EQ(min_int, std::numeric_limits<int>::min());
    EXPECT_FLOAT_EQ(max_float, std::numeric_limits<float>::max());
    EXPECT_FLOAT_EQ(min_float, std::numeric_limits<float>::lowest());
}

TEST_F(JsonArchiveTest, PropertyOverwrite)
{
    archive.add_property("duplicate", 10);
    archive.add_property("duplicate", 20);

    std::stringstream ss;
    archive.dump(ss);

    JsonArchive read_archive;
    read_archive.read(ss);

    int value;
    ASSERT_TRUE(read_archive.get_property("duplicate", value));
    EXPECT_EQ(value, 20);
}

// Performance and Stress Tests
TEST_F(JsonArchiveTest, DeepNesting)
{
    JsonArchive root;
    root.add_property("level0", 0);

    // Create nested structure
    ArchiveObject* current = &root;
    for (int i = 1; i < 10; ++i)
    {
        auto* child = current->create_child("nested");
        child->add_property("level", i);
        current = child;
    }

    std::stringstream ss;
    root.dump(ss);

    JsonArchive read_archive;
    read_archive.read(ss);

    int level0;
    ASSERT_TRUE(read_archive.get_property("level0", level0));
    EXPECT_EQ(level0, 0);
    ArchiveObject* read_current = &read_archive;
    for (int i = 1; i < 10; ++i)
    {
        auto* child = read_current->get_object("nested");
        ASSERT_NE(child, nullptr) << "Child at level " << i << " should not be null";
        int level;
        ASSERT_TRUE(child->get_property("level", level));
        EXPECT_EQ(level, i);
        read_current = child;
    }
}
