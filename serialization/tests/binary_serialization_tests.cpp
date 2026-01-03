//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//


#include <filesystem>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "portal/core/glm.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "portal/serialization/serialize/binary_serialization.h"

using namespace portal;
using namespace Catch::Matchers;

struct TestObject1
{
    int id = 0;
    std::string name{};
    glm::vec3 position{0.0f, 0.0f, 0.0f};

    void serialize(Serializer& s) const
    {
        s.add_value(id);
        s.add_value(name);
        s.add_value(position);
    }

    static TestObject1 deserialize(Deserializer& d)
    {
        TestObject1 obj{};
        d.get_value(obj.id);
        d.get_value(obj.name);
        d.get_value(obj.position);
        return obj;
    }

    bool operator==(const TestObject1& other) const
    {
        return id == other.id && name == other.name && position == other.position; //
    }
};

struct TestObjectNaked
{
    int id;
    glm::vec3 position;
    std::string name;

    bool operator==(const TestObjectNaked& other) const
    {
        return id == other.id && name == other.name && glm::all(glm::epsilonEqual(position, other.position, 0.001f));
    }
};

SCENARIO("BinarySerializer can serialize basic types")
{
    GIVEN("A BinarySerializer writing to a stringstream")
    {
        std::stringstream ss;
        BinarySerializer serializer(ss);

        THEN("Integer values are round-tripped correctly")
        {
            int original = 42;
            serializer.add_value(original);

            int deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);

            REQUIRE(deserialized == original);
        }

        THEN("Float values are round-tripped correctly")
        {
            float original = 3.14159f;
            serializer.add_value(original);

            float deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);

            REQUIRE_THAT(deserialized, WithinRel(original));
        }

        THEN("Double values are round-tripped correctly")
        {
            double original = 2.718281828;
            serializer.add_value(original);

            double deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);

            REQUIRE_THAT(deserialized, WithinRel(original));
        }

        THEN("Boolean values are round-tripped correctly")
        {
            bool original = true;
            serializer.add_value(original);

            bool deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);

            REQUIRE(deserialized == original);
        }


        THEN("Multiple basic types are round-tripped in sequence")
        {
            int i = 123;
            float f = 1.23f;
            bool b = false;
            serializer.add_value(i);
            serializer.add_value(f);
            serializer.add_value(b);

            int ri;
            float rf;
            bool rb;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(ri);
            deserializer.get_value(rf);
            deserializer.get_value(rb);

            REQUIRE(ri == i);
            REQUIRE_THAT(rf, WithinRel(f));
            REQUIRE(rb == b);
        }
    }
}

SCENARIO("BinarySerializer can serialize strings")
{
    GIVEN("A BinarySerializer writing to a stringstream")
    {
        std::stringstream ss;
        BinarySerializer serializer(ss);


        THEN("std::string is round-tripped")
        {
            std::string original = "Hello Binary World";
            serializer.add_value(original);

            std::string deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);

            REQUIRE(deserialized == original);
        }


        THEN("Empty string is round-tripped")
        {
            std::string original = "";
            serializer.add_value(original);

            std::string deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);

            REQUIRE(deserialized.empty());
        }

        THEN("String with special characters is round-tripped")
        {
            std::string special = "Line1\nLine2\tTabbed\0Null";
            serializer.add_value(special);

            std::string deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);

            REQUIRE(deserialized == special);
        }
    }
}

SCENARIO("BinarySerializer can serialize arrays")
{
    GIVEN("A BinarySerializer writing to a stringstream")
    {
        std::stringstream ss;
        BinarySerializer serializer(ss);

        THEN("std::vector<int> is round-tripped")
        {
            std::vector<int> original = {1, 2, 3, 4, 5};
            serializer.add_value(original);

            std::vector<int> deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);

            REQUIRE_THAT(deserialized, RangeEquals(original));
        }


        THEN("std::vector<float> is round-tripped")
        {
            std::vector<float> original = {1.1f, 2.2f, 3.3f};
            serializer.add_value(original);

            std::vector<float> deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);

            REQUIRE(deserialized.size() == original.size());
            for (size_t i = 0; i < original.size(); ++i)
            {
                REQUIRE_THAT(deserialized[i], WithinRel(original[i]));
            }
        }

        THEN("std::vector<std::string> is round-tripped")
        {
            std::vector<std::string> original = {"one", "two", "three"};
            serializer.add_value(original);

            std::vector<std::string> deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);

            REQUIRE_THAT(deserialized, RangeEquals(original));
        }

        THEN("Empty vector is round-tripped")
        {
            std::vector<int> original;
            serializer.add_value(original);

            std::vector<int> deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);

            REQUIRE(deserialized.empty());
        }
    }
}

SCENARIO("BinarySerializer can serialize GLM types")
{
    GIVEN("A BinarySerializer")
    {
        std::stringstream ss;
        BinarySerializer serializer(ss);

        THEN("glm::vec2 is round-tripped")
        {
            glm::vec2 original(1.0f, 2.0f);
            serializer.add_value(original);

            glm::vec2 deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);

            REQUIRE(original == deserialized);
        }

        THEN("glm::vec3 is round-tripped")
        {
            glm::vec3 original(1.0f, 2.0f, 3.0f);
            serializer.add_value(original);

            glm::vec3 deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);

            REQUIRE(original == deserialized);
        }

        THEN("glm::vec4 is round-tripped")
        {
            glm::vec4 original(1.0f, 2.0f, 3.0f, 4.0f);
            serializer.add_value(original);

            glm::vec4 deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);

            REQUIRE(original == deserialized);
        }

        THEN("glm::mat2 is round-tripped")
        {
            glm::mat2 original(1.0f, 2.0f, 3.0f, 4.0f);
            serializer.add_value(original);

            glm::mat2 deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);

            REQUIRE(original == deserialized);
        }

        THEN("glm::mat3 is round-tripped")
        {
            glm::mat3 original(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
            serializer.add_value(original);

            glm::mat3 deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);

            REQUIRE(original == deserialized);
        }

        THEN("glm::mat4 is round-tripped")
        {
            glm::mat4 original(1.0f);
            original[3] = glm::vec4(10.0f, 11.0f, 12.0f, 1.0f);
            serializer.add_value(original);

            glm::mat4 deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);

            REQUIRE(original == deserialized);
        }
    }
}

SCENARIO("BinarySerializer can serialize custom objects")
{
    GIVEN("A BinarySerializer")
    {
        std::stringstream ss;
        BinarySerializer serializer(ss);

        THEN("Custom object with serialization functions is round-tripped")
        {
            TestObject1 original{42, "Player1", {10.0f, 20.0f, 0.0f}};
            serializer.add_value(original);

            TestObject1 deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);

            REQUIRE(original == deserialized);
        }

        THEN("Vector of custom objects serialization functions is round-tripped")
        {
            std::vector<TestObject1> original{{1, "A", {1.0f, 0.0f, 0.0f}}, {2, "B", {0.0f, 2.0f, 0.0f}}};

            serializer.add_value(original);

            std::vector<TestObject1> deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);

            REQUIRE(original.size() == deserialized.size());
            REQUIRE(original[0] == deserialized[0]);
            REQUIRE(original[1] == deserialized[1]);
        }


        THEN("Custom object is round-tripped")
        {
            TestObjectNaked original{42, {10.0f, 20.0f, 0.0f}, "Player1"};
            serializer.add_value(original);

            TestObjectNaked deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);
            REQUIRE(original == deserialized);
        }

        THEN("Vector of custom objects is round-tripped")
        {
            std::vector<TestObjectNaked> original = {{1, {1.0f, 0.0f, 0.0f}, "A"}, {2, {0.0f, 2.0f, 0.0f}, "B"}};
            serializer.add_value(original);

            std::vector<TestObjectNaked> deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);

            REQUIRE(original.size() == deserialized.size());
            REQUIRE(original[0] == deserialized[0]);
            REQUIRE(original[1] == deserialized[1]);
        }
    }
}
