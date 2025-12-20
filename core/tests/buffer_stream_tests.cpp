//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "portal/core/buffer_stream.h"

TEST_CASE("BufferStreamReader operations", "[buffer_stream]")
{
    SECTION("basic read operations")
    {
        std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8};
        const portal::Buffer buffer(data.data(), data.size());

        portal::BufferStreamReader reader(buffer);
        REQUIRE(reader.position() == 0);

        uint8_t value;
        reader.read(reinterpret_cast<char*>(&value), sizeof(value));
        REQUIRE(value == 1);
        REQUIRE(reader.position() == 1);

        reader.read(reinterpret_cast<char*>(&value), sizeof(value));
        REQUIRE(value == 2);
        REQUIRE(reader.position() == 2);
    }

    SECTION("read multiple bytes")
    {
        std::vector<uint8_t> data{10, 20, 30, 40, 50};
        const portal::Buffer buffer(data.data(), data.size());

        portal::BufferStreamReader reader(buffer);

        std::vector<uint8_t> readBuffer(3);
        reader.read(reinterpret_cast<char*>(readBuffer.data()), 3);

        REQUIRE(readBuffer[0] == 10);
        REQUIRE(readBuffer[1] == 20);
        REQUIRE(readBuffer[2] == 30);
        REQUIRE(reader.position() == 3);
    }

    SECTION("seek operations")
    {
        std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8};
        const portal::Buffer buffer(data.data(), data.size());

        portal::BufferStreamReader reader(buffer);

        SECTION("seek from beginning")
        {
            reader.seekg(3, std::ios::beg);
            REQUIRE(reader.position() == 3);

            uint8_t value;
            reader.read(reinterpret_cast<char*>(&value), sizeof(value));
            REQUIRE(value == 4);
        }

        SECTION("seek from current position")
        {
            reader.seekg(3, std::ios::beg);
            uint8_t value;
            reader.read(reinterpret_cast<char*>(&value), sizeof(value));

            reader.seekg(-2, std::ios::cur);
            reader.read(reinterpret_cast<char*>(&value), sizeof(value));
            REQUIRE(value == 3);
        }

        SECTION("seek from end")
        {
            reader.seekg(-2, std::ios::end);
            uint8_t value;
            reader.read(reinterpret_cast<char*>(&value), sizeof(value));
            REQUIRE(value == 7);
        }
    }

    SECTION("read complex type")
    {
        struct TestStruct
        {
            int a;
            float b;
            char c;
        };

        TestStruct original{42, 3.14f, 'X'};
        const portal::Buffer buffer(&original, sizeof(TestStruct));

        portal::BufferStreamReader reader(buffer);

        TestStruct result{};
        reader.read(reinterpret_cast<char*>(&result), sizeof(TestStruct));

        REQUIRE(result.a == 42);
        REQUIRE_THAT(result.b, Catch::Matchers::WithinRel(3.14f, 0.0001f));
        REQUIRE(result.c == 'X');
    }

    SECTION("read zero bytes")
    {
        std::vector<uint8_t> data{1, 2, 3};
        const portal::Buffer buffer(data.data(), data.size());
        portal::BufferStreamReader reader(buffer);

        std::vector<uint8_t> read_buffer(3);
        reader.read(reinterpret_cast<char*>(read_buffer.data()), 0);
        REQUIRE(reader.position() == 0);
    }

    SECTION("read beyond end")
    {
        std::vector<uint8_t> data{1, 2, 3};
        const portal::Buffer buffer(data.data(), data.size());

        portal::BufferStreamReader reader(buffer);
        std::vector<uint8_t> read_buffer(5, 0);

        reader.read(reinterpret_cast<char*>(read_buffer.data()), 5);

        REQUIRE(read_buffer[0] == 1);
        REQUIRE(read_buffer[1] == 2);
        REQUIRE(read_buffer[2] == 3);
        REQUIRE(reader.eof());
    }

    SECTION("invalid seek operations")
    {
        std::vector<uint8_t> data{1, 2, 3, 4};
        const portal::Buffer buffer(data.data(), data.size());

        portal::BufferStreamReader reader(buffer);

        SECTION("seek beyond end")
        {
            reader.seekg(10, std::ios::beg);
            REQUIRE(reader.fail());
        }

        SECTION("seek before beginning")
        {
            reader.seekg(-2, std::ios::beg);
            REQUIRE(reader.fail());
        }

        SECTION("valid seek after clearing error state")
        {
            reader.seekg(10, std::ios::beg);
            REQUIRE(reader.fail());
            reader.clear();

            reader.seekg(2, std::ios::beg);
            REQUIRE(reader.position() == 2);
        }
    }
}

TEST_CASE("BufferStreamWriter operations", "[buffer_stream]")
{
    SECTION("basic write operations")
    {
        portal::Buffer buffer = portal::Buffer::allocate(2);

        portal::BufferStreamWriter writer(buffer);
        REQUIRE(writer.size() == 0);
        REQUIRE_FALSE(writer.full());

        uint8_t value = 42;
        writer.write(reinterpret_cast<const char*>(&value), sizeof(value));
        REQUIRE(writer.size() == 1);
        REQUIRE(buffer[0] == 42);

        value = 123;
        writer.write(reinterpret_cast<const char*>(&value), sizeof(value));
        REQUIRE(writer.size() == 2);
        REQUIRE(buffer[1] == 123);
        REQUIRE(writer.full());
    }

    SECTION("write multiple bytes")
    {
        portal::Buffer buffer = portal::Buffer::allocate(3);

        portal::BufferStreamWriter writer(buffer);

        const std::vector<uint8_t> data{10, 20, 30};
        writer.write(reinterpret_cast<const char*>(data.data()), data.size());

        REQUIRE(buffer[0] == 10);
        REQUIRE(buffer[1] == 20);
        REQUIRE(buffer[2] == 30);
        REQUIRE(writer.size() == 3);
        REQUIRE(writer.full());
    }

    SECTION("write complex type")
    {
        struct TestStruct
        {
            int a;
            float b;
            char c;
        };

        const TestStruct data{42, 3.14f, 'X'};

        portal::Buffer buffer = portal::Buffer::allocate(sizeof(TestStruct));

        portal::BufferStreamWriter writer(buffer);
        writer.write(reinterpret_cast<const char*>(&data), sizeof(TestStruct));

        const TestStruct* result = buffer.as<TestStruct*>();
        REQUIRE(result->a == 42);
        REQUIRE_THAT(result->b, Catch::Matchers::WithinRel(3.14f, 0.0001f));
        REQUIRE(result->c == 'X');
    }

    SECTION("full condition prevents further writes")
    {
        portal::Buffer buffer = portal::Buffer::allocate(2);

        portal::BufferStreamWriter writer(buffer);

        const std::vector<uint8_t> data{10, 20};
        writer.write(reinterpret_cast<const char*>(data.data()), data.size());

        REQUIRE(writer.full());

        uint8_t extra = 30;
        writer.write(reinterpret_cast<const char*>(&extra), sizeof(extra));
        REQUIRE(writer.size() == 2);
        REQUIRE(buffer[0] == 10);
        REQUIRE(buffer[1] == 20);
    }

    SECTION("get buffer returns correct size")
    {
        portal::Buffer buffer = portal::Buffer::allocate(10);

        portal::BufferStreamWriter writer(buffer);
        const std::vector<uint8_t> data{1, 2, 3, 4};
        writer.write(reinterpret_cast<const char*>(data.data()), data.size());

        const portal::Buffer result_buffer = writer.get_buffer();
        REQUIRE(result_buffer.size == 4);

        for (size_t i = 0; i < data.size(); ++i)
        {
            REQUIRE(result_buffer[i] == data[i]);
        }
    }
}

TEST_CASE("Empty buffer operations", "[buffer_stream]")
{
    portal::Buffer buffer;

    SECTION("reading from empty buffer")
    {
        portal::BufferStreamReader reader(buffer);
        uint8_t value = 123;
        reader.read(reinterpret_cast<char*>(&value), sizeof(value));
        REQUIRE(value == 123);
    }

    SECTION("writing to empty buffer")
    {
        portal::BufferStreamWriter writer(buffer);
        uint8_t value = 123;
        writer.write(reinterpret_cast<const char*>(&value), sizeof(value));
        REQUIRE(writer.size() == 0);
    }
}

TEST_CASE("BufferStream write and read integration", "[buffer_stream]")
{
    portal::Buffer buffer = portal::Buffer::allocate(10);

    SECTION("write then read")
    {
        {
            portal::BufferStreamWriter writer(buffer);
            const std::vector<uint8_t> data{10, 20, 30, 40};
            writer.write(reinterpret_cast<const char*>(data.data()), data.size());
        }

        {
            portal::BufferStreamReader reader(buffer);
            std::vector<uint8_t> read_data(4);
            reader.read(reinterpret_cast<char*>(read_data.data()), 4);

            REQUIRE(read_data[0] == 10);
            REQUIRE(read_data[1] == 20);
            REQUIRE(read_data[2] == 30);
            REQUIRE(read_data[3] == 40);
        }
    }
}
