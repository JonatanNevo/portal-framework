//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <sstream>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <numeric>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include <portal/core/glm.h>

#include "portal/serialization/archive/json_archive.h"
#include "portal/core/files/file_system.h"

using namespace portal;
using namespace Catch::Matchers;

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

// Helper to create and cleanup test directory
struct TestDirectory
{
    std::filesystem::path path;

    TestDirectory()
        : path(std::filesystem::temp_directory_path() / "json_archive_tests")
    {
        std::filesystem::create_directories(path);
    }

    ~TestDirectory()
    {
        if (std::filesystem::exists(path))
        {
            std::filesystem::remove_all(path);
        }
    }
};

SCENARIO("JsonArchive can serialize basic types")
{
    GIVEN("A JsonArchive")
    {
        JsonArchive archive;

        THEN("Integer values are serialized correctly")
        {
            archive.add_property("int_value", 42);

            std::stringstream ss;
            archive.dump(ss, 0);
            std::string json_output = ss.str();

            REQUIRE_FALSE(json_output.empty());
            REQUIRE(json_output.find("\"int_value\":42") != std::string::npos);
        }

        THEN("Float values are serialized correctly")
        {
            archive.add_property("float_value", 3.14f);

            std::stringstream ss;
            archive.dump(ss, 0);
            std::string json_output = ss.str();

            REQUIRE(json_output.find("\"float_value\":3.14") != std::string::npos);
        }

        THEN("Boolean values are serialized correctly")
        {
            archive.add_property("bool_value", true);

            std::stringstream ss;
            archive.dump(ss, 0);
            std::string json_output = ss.str();

            REQUIRE(json_output.find("\"bool_value\":true") != std::string::npos);
        }

        THEN("String values are serialized correctly")
        {
            archive.add_property("string_value", std::string("hello world"));

            std::stringstream ss;
            archive.dump(ss, 0);
            std::string json_output = ss.str();

            REQUIRE(json_output.find("\"string_value\":\"hello world\"") != std::string::npos);
        }

        THEN("Multiple basic types are serialized together")
        {
            archive.add_property("int_value", 42);
            archive.add_property("float_value", 3.14f);
            archive.add_property("double_value", 2.71828);
            archive.add_property("bool_value", true);
            archive.add_property("string_value", std::string("hello world"));

            std::stringstream ss;
            archive.dump(ss, 0);
            std::string json_output = ss.str();

            REQUIRE_FALSE(json_output.empty());
            REQUIRE(json_output.find("\"int_value\":42") != std::string::npos);
            REQUIRE(json_output.find("\"float_value\":3.14") != std::string::npos);
            REQUIRE(json_output.find("\"bool_value\":true") != std::string::npos);
            REQUIRE(json_output.find("\"string_value\":\"hello world\"") != std::string::npos);
        }
    }
}

SCENARIO("JsonArchive can serialize array types")
{
    GIVEN("A JsonArchive")
    {
        JsonArchive archive;

        THEN("Integer arrays are serialized correctly")
        {
            const std::vector<int> int_array = {1, 2, 3, 4, 5};
            archive.add_property("int_array", int_array);

            std::stringstream ss;
            archive.dump(ss, 0);
            std::string json_output = ss.str();

            REQUIRE(json_output.find("[1,2,3,4,5]") != std::string::npos);
        }

        THEN("Float arrays are serialized correctly")
        {
            const std::vector<float> float_array = {1.1f, 2.2f, 3.3f};
            archive.add_property("float_array", float_array);

            std::stringstream ss;
            archive.dump(ss, 0);
            std::string json_output = ss.str();

            REQUIRE_FALSE(json_output.empty());
        }

        THEN("String arrays are serialized correctly")
        {
            const std::vector<std::string> string_array = {"first", "second", "third"};
            archive.add_property("string_array", string_array);

            std::stringstream ss;
            archive.dump(ss, 0);
            std::string json_output = ss.str();

            REQUIRE(json_output.find("\"string_array\":[\"first\",\"second\",\"third\"]") != std::string::npos);
        }
    }
}

SCENARIO("JsonArchive can serialize nested objects")
{
    GIVEN("A JsonArchive and test objects")
    {
        JsonArchive archive;
        TestArchiveObject obj1{100, "object1", {1.0f, 2.0f}};
        TestArchiveObject obj2{200, "object2", {3.0f, 4.0f}};

        THEN("Single nested object is serialized")
        {
            archive.add_property("test_object", obj1);

            std::stringstream ss;
            archive.dump(ss);
            std::string json_output = ss.str();

            REQUIRE(json_output.find("\"test_object\"") != std::string::npos);
            REQUIRE(json_output.find("\"object1\"") != std::string::npos);
        }

        THEN("Array of nested objects is serialized")
        {
            std::vector<TestArchiveObject> object_array = {obj1, obj2};
            archive.add_property("object_array", object_array);

            std::stringstream ss;
            archive.dump(ss);
            std::string json_output = ss.str();

            REQUIRE(json_output.find("\"object_array\"") != std::string::npos);
            REQUIRE(json_output.find("\"object1\"") != std::string::npos);
            REQUIRE(json_output.find("\"object2\"") != std::string::npos);

            THEN("Array of nested objects is deserialized")
            {
                JsonArchive read_archive;
                read_archive.read(ss);

                std::vector<TestArchiveObject> read_object_array;
                REQUIRE(read_archive.get_property("object_array", read_object_array));
                REQUIRE_THAT(read_object_array, RangeEquals({obj1, obj2}));
            }
        }
    }
}

SCENARIO("JsonArchive can write to and read from files")
{
    GIVEN("A JsonArchive and a temporary directory")
    {
        TestDirectory test_dir;
        JsonArchive archive;

        THEN("Archive data can be written to a file")
        {
            archive.add_property("test_value", 42);
            archive.add_property("test_string", std::string("file test"));

            auto output_path = test_dir.path / "test_output.json";
            archive.dump(output_path, 0);

            REQUIRE(std::filesystem::exists(output_path));

            std::ifstream file(output_path);
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

            REQUIRE(content.find("\"test_value\":42") != std::string::npos);
            REQUIRE(content.find("\"test_string\":\"file test\"") != std::string::npos);
        }

        THEN("Archive data can be read from a file")
        {
            std::string json_content = R"({
        "int_prop": 123,
        "string_prop": "test string",
        "array_prop": [1, 2, 3],
        "bool_prop": true,
        "float_prop": 3.14
    })";

            auto input_path = test_dir.path / "test_input.json";
            std::ofstream file(input_path);
            file << json_content;
            file.close();

            JsonArchive read_archive;
            read_archive.read(input_path);

            int int_val;
            std::string string_val;
            std::vector<int> array_val;
            bool bool_val;
            float float_val;

            REQUIRE(read_archive.get_property("int_prop", int_val));
            REQUIRE(read_archive.get_property("string_prop", string_val));
            REQUIRE(read_archive.get_property("array_prop", array_val));
            REQUIRE(read_archive.get_property("bool_prop", bool_val));
            REQUIRE(read_archive.get_property("float_prop", float_val));

            REQUIRE(int_val == 123);
            REQUIRE_THAT(string_val, Equals("test string"));
            REQUIRE_THAT(array_val, RangeEquals(std::vector<int>{1, 2, 3}));
            REQUIRE(bool_val == true);
            REQUIRE_THAT(float_val, WithinRel(3.14f));
        }
    }
}

SCENARIO("JsonArchive can read from streams")
{
    GIVEN("A JSON stream with nested objects")
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

        THEN("Archive can read nested data from stream")
        {
            read_archive.read(ss);

            std::string simple_val;
            TestArchiveObject nested_obj;

            REQUIRE(read_archive.get_property("nested", nested_obj));
            REQUIRE(read_archive.get_property("simple", simple_val));
            REQUIRE_THAT(simple_val, Equals("value"));
            REQUIRE(nested_obj.value == 42);
            REQUIRE_THAT(nested_obj.name, Equals("nested_object"));
        }
    }
}

SCENARIO("JsonArchive supports round-trip serialization for basic types")
{
    GIVEN("A JsonArchive with basic types")
    {
        JsonArchive write_archive;
        write_archive.add_property("int_val", 42);
        write_archive.add_property("float_val", 3.14f);
        write_archive.add_property("string_val", std::string("round trip"));
        write_archive.add_property("bool_val", true);

        THEN("Data can be written and read back successfully")
        {
            std::stringstream ss;
            write_archive.dump(ss);

            JsonArchive read_archive;
            read_archive.read(ss);

            int int_val;
            float float_val;
            std::string string_val;
            bool bool_val;

            REQUIRE(read_archive.get_property("int_val", int_val));
            REQUIRE(read_archive.get_property("float_val", float_val));
            REQUIRE(read_archive.get_property("string_val", string_val));
            REQUIRE(read_archive.get_property("bool_val", bool_val));

            REQUIRE(int_val == 42);
            REQUIRE_THAT(float_val, WithinRel(3.14f));
            REQUIRE_THAT(string_val, Equals("round trip"));
            REQUIRE(bool_val == true);
        }
    }
}

SCENARIO("JsonArchive supports round-trip serialization for arrays")
{
    GIVEN("A JsonArchive with array data")
    {
        JsonArchive write_archive;
        std::vector<int> int_array = {1, 2, 3, 4, 5};
        std::vector<std::string> string_array = {"a", "b", "c"};

        write_archive.add_property("int_array", int_array);
        write_archive.add_property("string_array", string_array);

        THEN("Arrays can be written and read back successfully")
        {
            std::stringstream ss;
            write_archive.dump(ss);

            JsonArchive read_archive;
            read_archive.read(ss);

            std::vector<int> read_int_array;
            std::vector<std::string> read_string_array;

            REQUIRE(read_archive.get_property("int_array", read_int_array));
            REQUIRE(read_archive.get_property("string_array", read_string_array));

            REQUIRE_THAT(read_int_array, RangeEquals(int_array));
            REQUIRE_THAT(read_string_array, RangeEquals(string_array));
        }
    }
}

SCENARIO("JsonArchive supports round-trip serialization for complex objects")
{
    GIVEN("A JsonArchive with a complex object")
    {
        TestArchiveObject original{42, "test object", {1.0f, 2.0f, 3.0f}};

        JsonArchive write_archive;
        write_archive.add_property("test_obj", original);

        THEN("Complex object can be written and read back successfully")
        {
            std::stringstream ss;
            write_archive.dump(ss);

            JsonArchive read_archive;
            read_archive.read(ss);

            TestArchiveObject read_obj;
            REQUIRE(read_archive.get_property("test_obj", read_obj));
            REQUIRE(read_obj == original);
        }
    }
}

SCENARIO("JsonArchive handles edge cases")
{
    GIVEN("A JsonArchive")
    {
        JsonArchive archive;

        THEN("Empty archive serializes to empty object")
        {
            std::stringstream ss;
            archive.dump(ss);

            const std::string json_output = ss.str();
            REQUIRE(json_output == "{}");
        }

        THEN("Empty containers are handled correctly")
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

            REQUIRE(read_archive.get_property("empty_vector", read_vector));
            REQUIRE(read_archive.get_property("empty_string", read_string));
            REQUIRE(read_archive.get_property("empty_map", read_map));

            REQUIRE(read_vector.empty());
            REQUIRE(read_string.empty());
            REQUIRE(read_map.empty());
        }

        THEN("Property name conflicts result in overwrite")
        {
            archive.add_property("duplicate", 10);
            archive.add_property("duplicate", 20);

            std::stringstream ss;
            archive.dump(ss);

            JsonArchive read_archive;
            read_archive.read(ss);

            int value;
            REQUIRE(read_archive.get_property("duplicate", value));
            REQUIRE(value == 20);
        }
    }
}

SCENARIO("JsonArchive handles file errors gracefully")
{
    GIVEN("A JsonArchive and a test directory")
    {
        TestDirectory test_dir;
        JsonArchive archive;

        THEN("Non-existent file read does not crash")
        {
            const auto non_existent_path = test_dir.path / "non_existent.json";

            archive.read(non_existent_path);

            int dummy;
            REQUIRE_FALSE(archive.get_property("any_prop", dummy));
        }
    }
}

SCENARIO("JsonArchive handles invalid JSON gracefully")
{
    GIVEN("Invalid JSON input")
    {
        const std::string invalid_json = "{ invalid json content }";
        std::stringstream ss(invalid_json);

        THEN("Invalid JSON does not crash")
        {
            JsonArchive read_archive;
            read_archive.read(ss);

            int dummy;
            REQUIRE_FALSE(read_archive.get_property("any_prop", dummy));
        }
    }
}

SCENARIO("JsonArchive handles large data sets")
{
    GIVEN("A JsonArchive and a large array")
    {
        JsonArchive archive;
        std::vector<int> large_array(10000);
        std::iota(large_array.begin(), large_array.end(), 0);

        THEN("Large data sets are handled correctly")
        {
            archive.add_property("large_array", large_array);

            std::stringstream ss;
            archive.dump(ss);

            JsonArchive read_archive;
            read_archive.read(ss);

            std::vector<int> read_array;
            REQUIRE(read_archive.get_property("large_array", read_array));
            REQUIRE_THAT(read_array, RangeEquals(large_array));
        }
    }
}

SCENARIO("JsonArchive handles special characters in strings")
{
    GIVEN("A JsonArchive")
    {
        JsonArchive archive;

        THEN("Special characters are escaped correctly")
        {
            const std::string special_string = "Special chars: \n\t\r\"\\";
            archive.add_property("special", special_string);

            std::stringstream ss;
            archive.dump(ss);

            JsonArchive read_archive;
            read_archive.read(ss);

            std::string read_string;
            REQUIRE(read_archive.get_property("special", read_string));
            REQUIRE_THAT(read_string, Equals(special_string));
        }

        THEN("Unicode strings are preserved")
        {
            const std::string unicode_string = "Unicode: 你好 🌍 αβγ שלום עולם";
            archive.add_property("unicode", unicode_string);

            std::stringstream ss;
            archive.dump(ss);

            JsonArchive read_archive;
            read_archive.read(ss);

            std::string read_string;
            REQUIRE(read_archive.get_property("unicode", read_string));
            REQUIRE_THAT(read_string, Equals(unicode_string));
        }
    }
}

SCENARIO("JsonArchive handles numerical limits")
{
    GIVEN("A JsonArchive with extreme numerical values")
    {
        JsonArchive archive;
        archive.add_property("max_int", std::numeric_limits<int>::max());
        archive.add_property("min_int", std::numeric_limits<int>::min());
        archive.add_property("max_float", std::numeric_limits<float>::max());
        archive.add_property("min_float", std::numeric_limits<float>::lowest());

        THEN("Numerical limits are preserved in round-trip")
        {
            std::stringstream ss;
            archive.dump(ss);

            JsonArchive read_archive;
            read_archive.read(ss);

            int max_int, min_int;
            float max_float, min_float;

            REQUIRE(read_archive.get_property("max_int", max_int));
            REQUIRE(read_archive.get_property("min_int", min_int));
            REQUIRE(read_archive.get_property("max_float", max_float));
            REQUIRE(read_archive.get_property("min_float", min_float));

            REQUIRE(max_int == std::numeric_limits<int>::max());
            REQUIRE(min_int == std::numeric_limits<int>::min());
            REQUIRE_THAT(max_float, WithinRel(std::numeric_limits<float>::max()));
            REQUIRE_THAT(min_float, WithinRel(std::numeric_limits<float>::lowest()));
        }
    }
}

SCENARIO("JsonArchive handles deep nesting")
{
    GIVEN("A JsonArchive with deeply nested structure")
    {
        JsonArchive root;
        root.add_property("level0", 0);

        ArchiveObject* current = &root;
        for (int i = 1; i < 10; ++i)
        {
            auto* child = current->create_child("nested");
            child->add_property("level", i);
            current = child;
        }

        THEN("Deep nesting is preserved in round-trip")
        {
            std::stringstream ss;
            root.dump(ss);

            JsonArchive read_archive;
            read_archive.read(ss);

            int level0;
            REQUIRE(read_archive.get_property("level0", level0));
            REQUIRE(level0 == 0);

            ArchiveObject* read_current = &read_archive;
            for (int i = 1; i < 10; ++i)
            {
                auto* child = read_current->get_object("nested");
                REQUIRE(child != nullptr);

                int level;
                REQUIRE(child->get_property("level", level));
                REQUIRE(level == i);
                read_current = child;
            }
        }
    }
}

SCENARIO("JsonArchive supports round-trip serialization for GLM matrices")
{
    GIVEN("A JsonArchive with GLM matrices")
    {
        JsonArchive write_archive;

        THEN("mat2 can be written and read back successfully")
        {
            glm::mat2 original(1.0f, 2.0f, 3.0f, 4.0f);
            write_archive.add_property("mat2_val", original);

            std::stringstream ss;
            write_archive.dump(ss);

            JsonArchive read_archive;
            read_archive.read(ss);

            glm::mat2 retrieved;
            REQUIRE(read_archive.get_property("mat2_val", retrieved));
            for (int col = 0; col < 2; ++col)
            {
                for (int row = 0; row < 2; ++row)
                {
                    REQUIRE(retrieved[col][row] == original[col][row]);
                }
            }
        }

        THEN("mat3 can be written and read back successfully")
        {
            glm::mat3 original(
                1.0f, 2.0f, 3.0f,
                4.0f, 5.0f, 6.0f,
                7.0f, 8.0f, 9.0f
            );
            write_archive.add_property("mat3_val", original);

            std::stringstream ss;
            write_archive.dump(ss);

            JsonArchive read_archive;
            read_archive.read(ss);

            glm::mat3 retrieved;
            REQUIRE(read_archive.get_property("mat3_val", retrieved));
            for (int col = 0; col < 3; ++col)
            {
                for (int row = 0; row < 3; ++row)
                {
                    REQUIRE(retrieved[col][row] == original[col][row]);
                }
            }
        }

        THEN("mat4 can be written and read back successfully")
        {
            glm::mat4 original(
                1.0f, 2.0f, 3.0f, 4.0f,
                5.0f, 6.0f, 7.0f, 8.0f,
                9.0f, 10.0f, 11.0f, 12.0f,
                13.0f, 14.0f, 15.0f, 16.0f
            );
            write_archive.add_property("mat4_val", original);

            std::stringstream ss;
            write_archive.dump(ss);

            JsonArchive read_archive;
            read_archive.read(ss);

            glm::mat4 retrieved;
            REQUIRE(read_archive.get_property("mat4_val", retrieved));
            for (int col = 0; col < 4; ++col)
            {
                for (int row = 0; row < 4; ++row)
                {
                    REQUIRE(retrieved[col][row] == original[col][row]);
                }
            }
        }

        THEN("dmat4 can be written and read back successfully")
        {
            glm::dmat4 original(
                1.0, 2.0, 3.0, 4.0,
                5.0, 6.0, 7.0, 8.0,
                9.0, 10.0, 11.0, 12.0,
                13.0, 14.0, 15.0, 16.0
            );
            write_archive.add_property("dmat4_val", original);

            std::stringstream ss;
            write_archive.dump(ss);

            JsonArchive read_archive;
            read_archive.read(ss);

            glm::dmat4 retrieved;
            REQUIRE(read_archive.get_property("dmat4_val", retrieved));
            for (int col = 0; col < 4; ++col)
            {
                for (int row = 0; row < 4; ++row)
                {
                    REQUIRE(retrieved[col][row] == original[col][row]);
                }
            }
        }

        THEN("Multiple matrices can be serialized together")
        {
            glm::mat2 mat2_val(1.0f, 2.0f, 3.0f, 4.0f);
            glm::mat3 mat3_val(
                1.0f, 2.0f, 3.0f,
                4.0f, 5.0f, 6.0f,
                7.0f, 8.0f, 9.0f
            );
            glm::mat4 mat4_val(
                1.0f, 2.0f, 3.0f, 4.0f,
                5.0f, 6.0f, 7.0f, 8.0f,
                9.0f, 10.0f, 11.0f, 12.0f,
                13.0f, 14.0f, 15.0f, 16.0f
            );

            write_archive.add_property("mat2", mat2_val);
            write_archive.add_property("mat3", mat3_val);
            write_archive.add_property("mat4", mat4_val);

            std::stringstream ss;
            write_archive.dump(ss);

            JsonArchive read_archive;
            read_archive.read(ss);

            glm::mat2 read_mat2;
            glm::mat3 read_mat3;
            glm::mat4 read_mat4;

            REQUIRE(read_archive.get_property("mat2", read_mat2));
            REQUIRE(read_archive.get_property("mat3", read_mat3));
            REQUIRE(read_archive.get_property("mat4", read_mat4));

            REQUIRE(read_mat2 == mat2_val);
            REQUIRE(read_mat3 == mat3_val);
            REQUIRE(read_mat4 == mat4_val);
        }
    }
}