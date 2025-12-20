//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <numeric>
#include <vector>
#include <string>
#include <unordered_map>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include <portal/core/glm.h>
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

using namespace Catch::Matchers;

SCENARIO("Archive can handle basic types")
{
    GIVEN("An archive object")
    {
        ArchiveObject archive;

        THEN("`add_property<bool>` is called")
        {
            constexpr bool value = true;
            archive.add_property("test_bool", value);

            THEN("A boolean is inserted")
            {
                bool retrieved;
                REQUIRE(archive.get_property("test_bool", retrieved));
                REQUIRE(retrieved == value);
            }
        }

        THEN("`add_property<int>` is called")
        {
            constexpr int value = 42;
            archive.add_property("test_int", value);

            THEN("An integer is inserted")
            {
                int retrieved;
                REQUIRE(archive.get_property("test_int", retrieved));
                REQUIRE(retrieved == value);
            }
        }

        THEN("`add_property<float>` is called")
        {
            constexpr float value = 3.14f;
            archive.add_property("test_float", value);

            THEN("A float is inserted")
            {
                float retrieved;
                REQUIRE(archive.get_property("test_float", retrieved));
                REQUIRE(retrieved == value);
            }
        }

        THEN("`add_property<double>` is called")
        {
            constexpr double value = 2.71828;
            archive.add_property("test_double", value);

            THEN("A double is inserted")
            {
                double retrieved;
                REQUIRE(archive.get_property("test_double", retrieved));
                REQUIRE(retrieved == value);
            }
        }
    }
}

SCENARIO("Archive can handle complex types")
{
    GIVEN("An archive object")
    {
        ArchiveObject archive;

        THEN("`add_property<uint128_t>` is called")
        {
            constexpr uint128_t value = 12345678901234567890ULL;
            archive.add_property("test_uint128", value);

            THEN("A uint128_t is inserted")
            {
                uint128_t retrieved;
                REQUIRE(archive.get_property("test_uint128", retrieved));
                REQUIRE(retrieved == value);
            }
        }

        THEN("`add_property<std::vector>` is called")
        {
            const std::vector<int> int_values = {1, 2, 3, 4, 5};
            const std::vector<float> float_values = {1.1, 2.2, 3.3, 4.4, 5.5};
            const std::vector<std::string> string_values = {"first", "second", "third"};
            archive.add_property("int_vector", int_values);
            archive.add_property("float_vector", float_values);
            archive.add_property("string_vector", string_values);

            THEN("A vector is inserted")
            {
                std::vector<int> retrieved_int;
                std::vector<float> retrieved_float;
                std::vector<std::string> retrieved_string;
                REQUIRE(archive.get_property("int_vector", retrieved_int));
                REQUIRE(archive.get_property("float_vector", retrieved_float));
                REQUIRE(archive.get_property("string_vector", retrieved_string));
                REQUIRE_THAT(retrieved_int, RangeEquals(int_values));
                REQUIRE_THAT(retrieved_float, RangeEquals(float_values));
                REQUIRE_THAT(retrieved_string, RangeEquals(string_values));
            }
        }

        THEN("`add_property<std::vector<std::string>>` is called")
        {
            const std::string value = "Hello, World!";
            archive.add_property("test_string", value);

            THEN("A string is inserted")
            {
                std::string retrieved;
                REQUIRE(archive.get_property("test_string", retrieved));
                REQUIRE_THAT(retrieved, Equals(value));
            }

            THEN("string data is copied")
            {
                {
                    std::vector<std::string> new_value = {"first", "second", "third"};
                    archive.add_property("vector", new_value);
                    new_value.clear();
                    new_value.emplace_back("something");
                    new_value.emplace_back("else");
                }

                std::vector<std::string> retrieved;
                REQUIRE(archive.get_property("vector", retrieved));
                std::vector<std::string> expected = {"first", "second", "third"};
                REQUIRE_THAT(retrieved, Equals(expected));
            }

        }

        THEN("`add_property<glm::vec2>` is called")
        {
            glm::vec2 value(1.0f, 2.0f);
            archive.add_property("test_vec2", value);

            THEN("A vec2 is inserted")
            {
                glm::vec2 retrieved;
                REQUIRE(archive.get_property("test_vec2", retrieved));
                REQUIRE(retrieved.x == value.x);
                REQUIRE(retrieved.y == value.y);
            }
        }

        THEN("`add_property<glm::vec3>` is called")
        {
            glm::vec3 value(1.0f, 2.0f, 3.0f);
            archive.add_property("test_vec3", value);

            THEN("A vec3 is inserted")
            {
                glm::vec3 retrieved;
                REQUIRE(archive.get_property("test_vec3", retrieved));
                REQUIRE(retrieved.x == value.x);
                REQUIRE(retrieved.y == value.y);
                REQUIRE(retrieved.z == value.z);
            }
        }

        THEN("`add_property<glm::vec4>` is called")
        {
            glm::vec4 value(1.0f, 2.0f, 3.0f, 4.0f);
            archive.add_property("test_vec4", value);

            THEN("A vec4 is inserted")
            {
                glm::vec4 retrieved;
                REQUIRE(archive.get_property("test_vec4", retrieved));
                REQUIRE(retrieved.x == value.x);
                REQUIRE(retrieved.y == value.y);
                REQUIRE(retrieved.z == value.z);
                REQUIRE(retrieved.w == value.w);
            }
        }

        THEN("`add_property<std::unordered_map>` is called")
        {
            const std::unordered_map<std::string, int> values = {
                {"key1", 10},
                {"key2", 20},
                {"key3", 30}
            };
            archive.add_property("test_map", values);

            THEN("A map is inserted")
            {
                std::unordered_map<std::string, int> retrieved;
                REQUIRE(archive.get_property("test_map", retrieved));
                REQUIRE_THAT(retrieved, RangeEquals(values));
            }
        }

        THEN("multiple add_proprty calls are made with varius type")
        {
            archive.add_property("int_prop", 42);
            archive.add_property("float_prop", 3.14f);
            archive.add_property("string_prop", std::string("hello"));
            archive.add_property("vector_prop", std::vector<int>{1, 2, 3});

            int int_val;
            float float_val;
            std::string string_val;
            std::vector<int> vector_val;

            REQUIRE(archive.get_property("int_prop", int_val));
            REQUIRE(archive.get_property("float_prop", float_val));
            REQUIRE(archive.get_property("string_prop", string_val));
            REQUIRE(archive.get_property("vector_prop", vector_val));

            REQUIRE(int_val == 42);
            REQUIRE_THAT(float_val, WithinRel(3.14f));
            REQUIRE(string_val ==  "hello");
            REQUIRE_THAT(vector_val, Equals(std::vector<int>{1, 2, 3}));
        }
    }
}

SCENARIO("Archive can handle binary data")
{
    GIVEN("An archive")
    {
        ArchiveObject archive;

        AND_GIVEN("Binary data")
        {
            const std::vector<std::byte> data = {(std::byte)0xAA, (std::byte)0xBB, (std::byte)0xCC, (std::byte)0xDD, (std::byte)0xEE};
            Buffer buffer = Buffer::allocate(data.size());
            buffer.write(data.data(), data.size());

            THEN("`add_binary_block` is called")
            {
                archive.add_binary_block("binary_data", buffer);

                THEN("Binary data is written as is to the archive")
                {
                    Buffer retrieved;
                    REQUIRE(archive.get_binary_block("binary_data", retrieved));
                    REQUIRE(retrieved.size ==  buffer.size);
                    REQUIRE(std::memcmp(retrieved.data, buffer.data, buffer.size) ==  0);
                }

                THEN("Binary data can be retrieved into vector")
                {
                    std::vector<std::byte> retrieved;
                    REQUIRE(archive.get_binary_block("binary_data", retrieved));
                    REQUIRE_THAT(retrieved, RangeEquals(data));
                }
            }
        }

        AND_GIVEN("Large binary data")
        {
            std::vector<std::byte> original_data(1024);
            for (size_t i = 0; i < original_data.size(); ++i)
            {
                original_data[i] = static_cast<std::byte>(i % 256);
            }

            THEN("`add_binary_block` is called with a large buffer")
            {
                archive.add_binary_block("large_binary", original_data);

                THEN("Large binary archiving data works correctly")
                {

                }    std::vector<std::byte> retrieved_data;
                REQUIRE(archive.get_binary_block("large_binary", retrieved_data));
                REQUIRE_THAT(retrieved_data, RangeEquals(original_data));
            }
        }
    }
}

SCENARIO("Archive can handle custom types")
{
    GIVEN("An archive object")
    {
        ArchiveObject archive;

        AND_GIVEN("A custom object")
        {
            const TestObject obj{42, "test_object"};

            THEN("`add_property` is called")
            {
                archive.add_property("test_obj", obj);

                THEN("The custom object is archived correctly")
                {
                    TestObject retrieved;

                    REQUIRE(archive.get_property("test_obj", retrieved));
                    REQUIRE(retrieved.value == obj.value);
                    REQUIRE_THAT(retrieved.name, Equals(obj.name));
                }
            }
        }

        AND_GIVEN("Nested custom objects")
        {
            const TestObject first_object{100, "first"};
            const TestObject second_object{200, "second"};

            THEN("`add_property` is called on a vector of custom objects")
            {
                const std::vector vec_objects = {first_object, second_object};

                archive.add_property("nested_objects_vec", vec_objects);

                THEN("The nested objects are written correctly to the archive")
                {
                    std::vector<TestObject> retrieved_vec;
                    REQUIRE(archive.get_property("nested_objects_vec", retrieved_vec));
                    REQUIRE(retrieved_vec.size() ==  vec_objects.size());
                    for (size_t i = 0; i < vec_objects.size(); ++i)
                    {
                        REQUIRE(retrieved_vec[i].value ==  vec_objects[i].value);
                        REQUIRE_THAT(retrieved_vec[i].name, Equals(vec_objects[i].name));
                    }
                }
            }

            THEN("`add_property` is called on a map of custom objects")
            {
                const std::unordered_map<std::string, TestObject> objects = {{"first", first_object}, {"second", second_object}};
                archive.add_property("nested_objects", objects);

                std::unordered_map<std::string, TestObject> retrieved;
                REQUIRE(archive.get_property("nested_objects", retrieved));
                REQUIRE(retrieved.size() ==  objects.size());
                for (const auto& [key, value] : objects)
                {
                    auto it = retrieved.find(key);
                    REQUIRE_FALSE(it ==  retrieved.end());
                    REQUIRE(it->second.value ==  value.value);
                    REQUIRE_THAT(it->second.name, Equals(value.name));
                }

            }
        }
    }
}

SCENARIO("Archive handles empty containers corretly")
{
    GIVEN("An archive object")
    {
        ArchiveObject archive;

        THEN("adding empty vector is handled")
        {
            const std::vector<int> empty_vector;
            archive.add_property("empty_vector", empty_vector);

            std::vector<int> retrieved;
            REQUIRE(archive.get_property("empty_vector", retrieved));
            REQUIRE(retrieved.empty());
        }

        THEN("adding empty string is handled")
        {
            const std::string empty_string;
            archive.add_property("empty_string", empty_string);

            std::string retrieved;
            REQUIRE(archive.get_property("empty_string", retrieved));
            REQUIRE(retrieved.empty());
        }

        THEN("adding empty map is handled")
        {
            const std::unordered_map<std::string, int> empty_map;
            archive.add_property("empty_map", empty_map);

            std::unordered_map<std::string, int> retrieved;
            REQUIRE(archive.get_property("empty_map", retrieved));
            REQUIRE(retrieved.empty());
        }
    }
}

SCENARIO("Archiving large data sets")
{
    GIVEN("An archive object and large data set")
    {
        ArchiveObject archive;

        std::vector<int> large_vector(10000);
        std::iota(large_vector.begin(), large_vector.end(), 0);

        THEN("Archive can handle large data sets")
        {
            archive.add_property("large_vector", large_vector);

            std::vector<int> retrieved;
            REQUIRE(archive.get_property("large_vector", retrieved));
            REQUIRE(retrieved.size() ==  large_vector.size());
            REQUIRE_THAT(retrieved, RangeEquals(large_vector));
        }
    }
}

SCENARIO("Archive recovers from invalid input correctly")
{
    GIVEN("An archive object")
    {
        ArchiveObject archive;

        THEN("Property name conflicts are not fatal")
        {
            archive.add_property("duplicate", 10);
            archive.add_property("duplicate", 20); // Should overwrite

            int retrieved;
            REQUIRE(archive.get_property("duplicate", retrieved));
            REQUIRE(retrieved ==  20);
        }

        THEN("Invalid property access is not fatal")
        {
            int value;
            REQUIRE_FALSE(archive.get_property("non_existent_property", value));
        }

        THEN("Strings containing null bytes are handles correctly")
        {
            std::string test_string = "test\0embedded\0nulls";
            archive.add_property("null_string", test_string);

            std::string retrieved;
            REQUIRE(archive.get_property("null_string", retrieved));

            REQUIRE(retrieved.length() ==  test_string.length());
            REQUIRE_THAT(retrieved, RangeEquals(test_string));
        }
    }
}