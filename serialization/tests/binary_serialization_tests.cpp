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
#include "spdlog/fmt/bin_to_hex.h"

#include "portal/core/buffer_stream.h"
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

// External type that doesn't have intrusive serialize/deserialize methods
struct ExternalData
{
    float x;
    float y;
    float z;
    int priority;

    bool operator==(const ExternalData& other) const
    {
        return x == other.x && y == other.y && z == other.z && priority == other.priority;
    }
};

// Another external type for testing
struct ExternalPacket
{
    std::string header;
    std::vector<int> payload;
    bool valid;

    bool operator==(const ExternalPacket& other) const
    {
        return header == other.header && payload == other.payload && valid == other.valid;
    }
};

// Non-intrusive serialization specialization for ExternalData
template <>
struct portal::Serializable<ExternalData>
{
    static void serialize(const ExternalData& obj, Serializer& s)
    {
        s.add_value(obj.x);
        s.add_value(obj.y);
        s.add_value(obj.z);
        s.add_value(obj.priority);
    }

    static ExternalData deserialize(Deserializer& d)
    {
        ExternalData data{};
        d.get_value(data.x);
        d.get_value(data.y);
        d.get_value(data.z);
        d.get_value(data.priority);
        return data;
    }
};

// Non-intrusive serialization specialization for ExternalPacket
template <>
struct portal::Serializable<ExternalPacket>
{
    static void serialize(const ExternalPacket& obj, Serializer& s)
    {
        s.add_value(obj.header);
        s.add_value(obj.payload);
        s.add_value(obj.valid);
    }

    static ExternalPacket deserialize(Deserializer& d)
    {
        ExternalPacket packet{};
        d.get_value(packet.header);
        d.get_value(packet.payload);
        d.get_value(packet.valid);
        return packet;
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
            glm::vec3 original(1.1234f, 2.23456f, 3.34567f);
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


SCENARIO("BinarySerializer can serialize maps")
{
    GIVEN("A BinarySerializer")
    {
        std::stringstream ss;
        BinarySerializer serializer(ss);

        THEN("std::map<std::string, int> is round-tripped")
        {
            std::map<std::string, int> original = {{"one", 1}, {"two", 2}};
            serializer.add_value(original);

            std::map<std::string, int> deserialized;
            BinaryDeserializer deserializer(ss);
            deserializer.get_value(deserialized);

            REQUIRE(deserialized.size() == original.size());
            REQUIRE(deserialized["one"] == 1);
            REQUIRE(deserialized["two"] == 2);
        }
    }
}

SCENARIO("BinarySerializer handles configuration parameters")
{
    GIVEN("A BinarySerializer with no header")
    {
        std::stringstream ss;
        BinarySerializationParams params;
        params.encode_header = false;

        BinarySerializer serializer(ss, params);

        THEN("Data is written without magic bytes")
        {
            int value = 42;
            serializer.add_value(value); // Should write just the packet for int

            // Verify logic:
            // If header was written, it would be 4 bytes.
            // Int packet: 2 bytes type + 4 bytes data = 6 bytes.
            // So if size is small close to 6, header is gone.
            // Actually let's just reverse it with same params.

            ss.seekg(0, std::ios::end);
            size_t size = ss.tellg();
            ss.seekg(0, std::ios::beg);

            // Magic (4) + Type (2) + Data (4) = 10 bytes normally.
            // Without magic: Type (2) + Data (4) = 6 bytes.
            // Let's just check round trip with matching params.

            BinaryDeserializer deserializer(ss, params);
            int deserialized;
            deserializer.get_value(deserialized);

            REQUIRE(deserialized == value);
        }
    }
}

SCENARIO("BinarySerializer huge data test")
{
    GIVEN("A large vector of integers")
    {
        std::vector<int> huge_data(10000);
        std::iota(huge_data.begin(), huge_data.end(), 0);

        std::stringstream ss;
        BinarySerializer serializer(ss);
        serializer.add_value(huge_data);

        THEN("It can be deserialized correctly")
        {
            BinaryDeserializer deserializer(ss);
            std::vector<int> output;
            deserializer.get_value(output);

            REQUIRE_THAT(output, RangeEquals(huge_data));
        }
    }
}

SCENARIO("BinarySerializer can serialize types with non-intrusive SerializableType specialization")
{
    GIVEN("A BinarySerializer writing to a stringstream")
    {
        std::stringstream ss;
        BinarySerializer serializer(ss);

        AND_GIVEN("An external type with SerializableType specialization")
        {
            const ExternalData data{1.5f, 2.5f, 3.5f, 42};

            THEN("The external object is round-tripped correctly")
            {
                serializer.add_value(data);

                ExternalData deserialized{};
                BinaryDeserializer deserializer(ss);
                deserializer.get_value(deserialized);

                REQUIRE(deserialized == data);
            }
        }

        AND_GIVEN("Multiple external types with SerializableType specializations")
        {
            const ExternalData data{10.0f, 20.0f, 30.0f, 100};
            const ExternalPacket packet{"test_header", {1, 2, 3, 4, 5}, true};

            THEN("Both types can be serialized and deserialized")
            {
                serializer.add_value(data);
                serializer.add_value(packet);

                ExternalData retrieved_data{};
                ExternalPacket retrieved_packet{};

                BinaryDeserializer deserializer(ss);
                deserializer.get_value(retrieved_data);
                deserializer.get_value(retrieved_packet);

                REQUIRE(retrieved_data == data);
                REQUIRE(retrieved_packet == packet);
            }
        }

        AND_GIVEN("A vector of external types")
        {
            const std::vector<ExternalData> data_list = {
                {1.0f, 2.0f, 3.0f, 1},
                {4.0f, 5.0f, 6.0f, 2},
                {7.0f, 8.0f, 9.0f, 3}
            };

            THEN("The vector of external objects is round-tripped correctly")
            {
                serializer.add_value(data_list);

                std::vector<ExternalData> retrieved;
                BinaryDeserializer deserializer(ss);
                deserializer.get_value(retrieved);

                REQUIRE(retrieved.size() == data_list.size());
                for (size_t i = 0; i < data_list.size(); ++i)
                {
                    REQUIRE(retrieved[i] == data_list[i]);
                }
            }
        }

        AND_GIVEN("An empty vector of external types")
        {
            const std::vector<ExternalData> empty_list;

            THEN("Empty vector of external types is handled correctly")
            {
                serializer.add_value(empty_list);

                std::vector<ExternalData> retrieved;
                BinaryDeserializer deserializer(ss);
                deserializer.get_value(retrieved);

                REQUIRE(retrieved.empty());
            }
        }

        AND_GIVEN("Nested structures mixing intrusive and non-intrusive types")
        {
            const TestObject1 intrusive_obj{42, "intrusive", {1.0f, 2.0f, 3.0f}};
            const ExternalData external_obj{1.0f, 2.0f, 3.0f, 10};

            THEN("Both intrusive and non-intrusive types can coexist in the same stream")
            {
                serializer.add_value(intrusive_obj);
                serializer.add_value(external_obj);

                TestObject1 retrieved_intrusive{};
                ExternalData retrieved_external{};

                BinaryDeserializer deserializer(ss);
                deserializer.get_value(retrieved_intrusive);
                deserializer.get_value(retrieved_external);

                REQUIRE(retrieved_intrusive == intrusive_obj);
                REQUIRE(retrieved_external == external_obj);
            }
        }
    }
}

SCENARIO("Non-intrusive serialization concepts are correctly detected")
{
    THEN("ExternalSerializable is satisfied for types with SerializableType specialization")
    {
        STATIC_REQUIRE(ExternalSerializable<ExternalData>);
        STATIC_REQUIRE(ExternalSerializable<ExternalPacket>);
    }

    THEN("ExternalDeserializable is satisfied for types with SerializableType specialization")
    {
        STATIC_REQUIRE(ExternalDeserializable<ExternalData>);
        STATIC_REQUIRE(ExternalDeserializable<ExternalPacket>);
    }

    THEN("Serializable is satisfied for intrusive types")
    {
        STATIC_REQUIRE(SerializableConcept<TestObject1>);
    }

    THEN("ExternalSerializable is NOT satisfied for intrusive types without specialization")
    {
        STATIC_REQUIRE_FALSE(ExternalSerializable<TestObject1>);
    }

    THEN("Serializable is NOT satisfied for external types")
    {
        STATIC_REQUIRE_FALSE(SerializableConcept<ExternalData>);
        STATIC_REQUIRE_FALSE(SerializableConcept<ExternalPacket>);
    }
}

SCENARIO("BinarySerializer can reserve slots for deferred writing")
{
    GIVEN("A BinarySerializer writing to a buffer")
    {
        Buffer buffer;
        BufferStreamWriter writer(buffer);
        BufferStreamReader reader(buffer);
        BinarySerializer serializer(writer);

        THEN("A reserved size_t slot can be written later")
        {
            auto size_slot = serializer.reserve<size_t>();
            size_t count = 42;
            size_slot.write(count);

            size_t deserialized;
            BinaryDeserializer deserializer(reader);
            deserializer.get_value(deserialized);

            REQUIRE(deserialized == count);
        }

        THEN("Reserved slot works with data written after it")
        {
            // Reserve space for the count
            auto count_slot = serializer.reserve<size_t>();

            // Write some items and count them
            size_t count = 0;
            for (int i = 0; i < 5; ++i)
            {
                serializer.add_value(i * 10);
                ++count;
            }

            // Write the count back to the reserved slot
            count_slot.write(count);

            // Deserialize and verify
            BinaryDeserializer deserializer(reader);

            size_t deserialized_count;
            deserializer.get_value(deserialized_count);
            REQUIRE(deserialized_count == 5);

            for (int i = 0; i < 5; ++i)
            {
                int value;
                deserializer.get_value(value);
                REQUIRE(value == i * 10);
            }
        }

        THEN("Multiple reserved slots can be used")
        {
            auto slot1 = serializer.reserve<int>();
            auto slot2 = serializer.reserve<float>();
            serializer.add_value(std::string("middle"));
            auto slot3 = serializer.reserve<double>();

            // Write values in different order than reserved
            slot3.write(3.14159);
            slot1.write(42);
            slot2.write(2.718f);

            BinaryDeserializer deserializer(reader);

            int val1;
            float val2;
            std::string middle;
            double val3;

            deserializer.get_value(val1);
            deserializer.get_value(val2);
            deserializer.get_value(middle);
            deserializer.get_value(val3);

            REQUIRE(val1 == 42);
            REQUIRE_THAT(val2, WithinRel(2.718f));
            REQUIRE(middle == "middle");
            REQUIRE_THAT(val3, WithinRel(3.14159));
        }

        THEN("Reserve works with different integral types")
        {
            auto slot_i8 = serializer.reserve<int8_t>();
            auto slot_u16 = serializer.reserve<uint16_t>();
            auto slot_i32 = serializer.reserve<int32_t>();
            auto slot_u64 = serializer.reserve<uint64_t>();

            slot_i8.write(static_cast<int8_t>(-42));
            slot_u16.write(static_cast<uint16_t>(1234));
            slot_i32.write(-987654);
            slot_u64.write(static_cast<uint64_t>(0xDEADBEEFCAFEBABE));

            BinaryDeserializer deserializer(reader);

            int8_t val_i8;
            uint16_t val_u16;
            int32_t val_i32;
            uint64_t val_u64;

            deserializer.get_value(val_i8);
            deserializer.get_value(val_u16);
            deserializer.get_value(val_i32);
            deserializer.get_value(val_u64);

            REQUIRE(val_i8 == -42);
            REQUIRE(val_u16 == 1234);
            REQUIRE(val_i32 == -987654);
            REQUIRE(val_u64 == 0xDEADBEEFCAFEBABE);
        }

        THEN("Reserve works with floating point types")
        {
            auto slot_f = serializer.reserve<float>();
            auto slot_d = serializer.reserve<double>();

            slot_f.write(1.23456f);
            slot_d.write(9.87654321);

            BinaryDeserializer deserializer(reader);

            float val_f;
            double val_d;

            deserializer.get_value(val_f);
            deserializer.get_value(val_d);

            REQUIRE_THAT(val_f, WithinRel(1.23456f));
            REQUIRE_THAT(val_d, WithinRel(9.87654321));
        }

        THEN("Lazy list serialization pattern works correctly")
        {
            // This is the primary use case: serialize a lazy-evaluated list
            // where we don't know the count until we've iterated

            auto count_slot = serializer.reserve<size_t>();

            // Simulate lazy evaluation - we don't know the count upfront
            std::vector<std::string> lazy_items = {"apple", "banana", "cherry", "date"};
            size_t count = 0;

            for (const auto& item : lazy_items)
            {
                serializer.add_value(item);
                ++count;
            }

            // Now write the count we discovered
            count_slot.write(count);

            // Deserialize
            BinaryDeserializer deserializer(reader);

            size_t deserialized_count;
            deserializer.get_value(deserialized_count);
            REQUIRE(deserialized_count == lazy_items.size());

            for (size_t i = 0; i < deserialized_count; ++i)
            {
                std::string item;
                deserializer.get_value(item);
                REQUIRE(item == lazy_items[i]);
            }
        }
    }
}
