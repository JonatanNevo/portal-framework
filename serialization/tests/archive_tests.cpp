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

// External type that doesn't have intrusive archive/dearchive methods
struct ExternalPoint
{
    float x;
    float y;
    float z;

    bool operator==(const ExternalPoint& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }
};

// Another external type for testing
struct ExternalConfig
{
    std::string name;
    int priority;
    bool enabled;

    bool operator==(const ExternalConfig& other) const
    {
        return name == other.name && priority == other.priority && enabled == other.enabled;
    }
};

// Non-intrusive archiving specialization for ExternalPoint
template <>
struct portal::Archivable<ExternalPoint>
{
    static void archive(const ExternalPoint& obj, ArchiveObject& ar)
    {
        ar.add_property("x", obj.x);
        ar.add_property("y", obj.y);
        ar.add_property("z", obj.z);
    }

    static ExternalPoint dearchive(ArchiveObject& ar)
    {
        ExternalPoint point{};
        ar.get_property("x", point.x);
        ar.get_property("y", point.y);
        ar.get_property("z", point.z);
        return point;
    }
};

// Non-intrusive archiving specialization for ExternalConfig
template <>
struct portal::Archivable<ExternalConfig>
{
    static void archive(const ExternalConfig& obj, ArchiveObject& ar)
    {
        ar.add_property("name", obj.name);
        ar.add_property("priority", obj.priority);
        ar.add_property("enabled", obj.enabled);
    }

    static ExternalConfig dearchive(ArchiveObject& ar)
    {
        ExternalConfig config{};
        ar.get_property("name", config.name);
        ar.get_property("priority", config.priority);
        ar.get_property("enabled", config.enabled);
        return config;
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

        THEN("`add_property<glm::mat2>` is called")
        {
            glm::mat2 value(1.0f, 2.0f, 3.0f, 4.0f);
            archive.add_property("test_mat2", value);

            THEN("A mat2 is inserted")
            {
                glm::mat2 retrieved;
                REQUIRE(archive.get_property("test_mat2", retrieved));
                REQUIRE(retrieved[0][0] == value[0][0]);
                REQUIRE(retrieved[0][1] == value[0][1]);
                REQUIRE(retrieved[1][0] == value[1][0]);
                REQUIRE(retrieved[1][1] == value[1][1]);
            }
        }

        THEN("`add_property<glm::mat3>` is called")
        {
            glm::mat3 value(
                1.0f, 2.0f, 3.0f,
                4.0f, 5.0f, 6.0f,
                7.0f, 8.0f, 9.0f
            );
            archive.add_property("test_mat3", value);

            THEN("A mat3 is inserted")
            {
                glm::mat3 retrieved;
                REQUIRE(archive.get_property("test_mat3", retrieved));
                for (int col = 0; col < 3; ++col)
                {
                    for (int row = 0; row < 3; ++row)
                    {
                        REQUIRE(retrieved[col][row] == value[col][row]);
                    }
                }
            }
        }

        THEN("`add_property<glm::mat4>` is called")
        {
            glm::mat4 value(
                1.0f, 2.0f, 3.0f, 4.0f,
                5.0f, 6.0f, 7.0f, 8.0f,
                9.0f, 10.0f, 11.0f, 12.0f,
                13.0f, 14.0f, 15.0f, 16.0f
            );
            archive.add_property("test_mat4", value);

            THEN("A mat4 is inserted")
            {
                glm::mat4 retrieved;
                REQUIRE(archive.get_property("test_mat4", retrieved));
                for (int col = 0; col < 4; ++col)
                {
                    for (int row = 0; row < 4; ++row)
                    {
                        REQUIRE(retrieved[col][row] == value[col][row]);
                    }
                }
            }
        }

        THEN("`add_property<glm::dmat4>` is called")
        {
            glm::dmat4 value(
                1.0, 2.0, 3.0, 4.0,
                5.0, 6.0, 7.0, 8.0,
                9.0, 10.0, 11.0, 12.0,
                13.0, 14.0, 15.0, 16.0
            );
            archive.add_property("test_dmat4", value);

            THEN("A dmat4 is inserted")
            {
                glm::dmat4 retrieved;
                REQUIRE(archive.get_property("test_dmat4", retrieved));
                for (int col = 0; col < 4; ++col)
                {
                    for (int row = 0; row < 4; ++row)
                    {
                        REQUIRE(retrieved[col][row] == value[col][row]);
                    }
                }
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

SCENARIO("Archive can handle types with non-intrusive Archivable specialization")
{
    GIVEN("An archive object")
    {
        ArchiveObject archive;

        AND_GIVEN("An external type with Archivable specialization")
        {
            const ExternalPoint point{1.5f, 2.5f, 3.5f};

            THEN("`add_property` is called on the external type")
            {
                archive.add_property("point", point);

                THEN("The external object is archived correctly")
                {
                    ExternalPoint retrieved{};
                    REQUIRE(archive.get_property("point", retrieved));
                    REQUIRE(retrieved.x == point.x);
                    REQUIRE(retrieved.y == point.y);
                    REQUIRE(retrieved.z == point.z);
                }
            }
        }

        AND_GIVEN("Multiple external types with Archivable specializations")
        {
            const ExternalPoint point{10.0f, 20.0f, 30.0f};
            const ExternalConfig config{"test_config", 5, true};

            THEN("Both types can be archived and dearchived")
            {
                archive.add_property("point", point);
                archive.add_property("config", config);

                ExternalPoint retrieved_point{};
                ExternalConfig retrieved_config{};

                REQUIRE(archive.get_property("point", retrieved_point));
                REQUIRE(archive.get_property("config", retrieved_config));

                REQUIRE(retrieved_point == point);
                REQUIRE(retrieved_config == config);
            }
        }

        AND_GIVEN("A vector of external types")
        {
            const std::vector<ExternalPoint> points = {
                {1.0f, 2.0f, 3.0f},
                {4.0f, 5.0f, 6.0f},
                {7.0f, 8.0f, 9.0f}
            };

            THEN("`add_property` is called on the vector")
            {
                archive.add_property("points", points);

                THEN("The vector of external objects is archived correctly")
                {
                    std::vector<ExternalPoint> retrieved;
                    REQUIRE(archive.get_property("points", retrieved));
                    REQUIRE(retrieved.size() == points.size());

                    for (size_t i = 0; i < points.size(); ++i)
                    {
                        REQUIRE(retrieved[i] == points[i]);
                    }
                }
            }
        }

        AND_GIVEN("An empty vector of external types")
        {
            const std::vector<ExternalPoint> empty_points;

            THEN("Empty vector of external types is handled correctly")
            {
                archive.add_property("empty_points", empty_points);

                std::vector<ExternalPoint> retrieved;
                REQUIRE(archive.get_property("empty_points", retrieved));
                REQUIRE(retrieved.empty());
            }
        }

        AND_GIVEN("Nested structures mixing intrusive and non-intrusive types")
        {
            const TestObject intrusive_obj{42, "intrusive"};
            const ExternalPoint external_obj{1.0f, 2.0f, 3.0f};

            THEN("Both intrusive and non-intrusive types can coexist in the same archive")
            {
                archive.add_property("intrusive", intrusive_obj);
                archive.add_property("external", external_obj);

                TestObject retrieved_intrusive;
                ExternalPoint retrieved_external{};

                REQUIRE(archive.get_property("intrusive", retrieved_intrusive));
                REQUIRE(archive.get_property("external", retrieved_external));

                REQUIRE(retrieved_intrusive.value == intrusive_obj.value);
                REQUIRE_THAT(retrieved_intrusive.name, Equals(intrusive_obj.name));
                REQUIRE(retrieved_external == external_obj);
            }
        }
    }
}

SCENARIO("Non-intrusive archiving concepts are correctly detected")
{
    THEN("ExternalArchiveableConcept is satisfied for types with Archivable specialization")
    {
        STATIC_REQUIRE(ExternalArchiveableConcept<ExternalPoint>);
        STATIC_REQUIRE(ExternalArchiveableConcept<ExternalConfig>);
    }

    THEN("ExternalDearchiveableConcept is satisfied for types with Archivable specialization")
    {
        STATIC_REQUIRE(ExternalDearchiveableConcept<ExternalPoint>);
        STATIC_REQUIRE(ExternalDearchiveableConcept<ExternalConfig>);
    }

    THEN("ArchiveableConcept is satisfied for intrusive types")
    {
        STATIC_REQUIRE(ArchiveableConcept<TestObject>);
    }

    THEN("ExternalArchiveableConcept is NOT satisfied for intrusive types without specialization")
    {
        STATIC_REQUIRE_FALSE(ExternalArchiveableConcept<TestObject>);
    }

    THEN("ArchiveableConcept is NOT satisfied for external types")
    {
        STATIC_REQUIRE_FALSE(ArchiveableConcept<ExternalPoint>);
        STATIC_REQUIRE_FALSE(ArchiveableConcept<ExternalConfig>);
    }
}