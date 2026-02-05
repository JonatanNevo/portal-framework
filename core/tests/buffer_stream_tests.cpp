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
        portal::Buffer buffer = portal::Buffer::allocate(10);
        portal::BufferStreamWriter writer(buffer);
        REQUIRE(writer.size() == 0);
        REQUIRE(writer.capacity() == 10);

        uint8_t value = 42;
        writer.write(reinterpret_cast<const char*>(&value), sizeof(value));
        REQUIRE(writer.size() == 1);

        const portal::Buffer result = writer.get_buffer();
        REQUIRE(result[0] == 42);

        value = 123;
        writer.write(reinterpret_cast<const char*>(&value), sizeof(value));
        REQUIRE(writer.size() == 2);

        const portal::Buffer result2 = writer.get_buffer();
        REQUIRE(result2[1] == 123);
    }

    SECTION("write multiple bytes")
    {
        portal::Buffer buffer = portal::Buffer::allocate(10);
        portal::BufferStreamWriter writer(buffer);

        const std::vector<uint8_t> data{10, 20, 30};
        writer.write(reinterpret_cast<const char*>(data.data()), data.size());

        const portal::Buffer result = writer.get_buffer();
        REQUIRE(result[0] == 10);
        REQUIRE(result[1] == 20);
        REQUIRE(result[2] == 30);
        REQUIRE(writer.size() == 3);
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

        const portal::Buffer result = writer.get_buffer();
        const TestStruct* result_ptr = result.as<TestStruct*>();
        REQUIRE(result_ptr->a == 42);
        REQUIRE_THAT(result_ptr->b, Catch::Matchers::WithinRel(3.14f, 0.0001f));
        REQUIRE(result_ptr->c == 'X');
    }


    SECTION("automatic growth on overflow")
    {
        portal::Buffer buffer = portal::Buffer::allocate(2);
        portal::BufferStreamWriter writer(buffer);
        REQUIRE(writer.capacity() == 2);

        const std::vector<uint8_t> data{10, 20, 30, 40, 50};
        writer.write(reinterpret_cast<const char*>(data.data()), data.size());

        REQUIRE(writer.size() == 5);
        REQUIRE(writer.capacity() >= 5);

        const portal::Buffer result = writer.get_buffer();
        REQUIRE(result.size == 5);
        for (size_t i = 0; i < data.size(); ++i)
        {
            REQUIRE(result[i] == data[i]);
        }
    }


    SECTION("automatic growth with single character overflow")
    {
        portal::Buffer buffer = portal::Buffer::allocate(2);
        portal::BufferStreamWriter writer(buffer);

        writer.put('A');
        writer.put('B');
        REQUIRE(writer.size() == 2);
        REQUIRE(writer.capacity() == 2);

        writer.put('C');
        REQUIRE(writer.size() == 3);
        REQUIRE(writer.capacity() >= 3);

        const portal::Buffer result = writer.get_buffer();
        REQUIRE(result[0] == 'A');
        REQUIRE(result[1] == 'B');
        REQUIRE(result[2] == 'C');
    }

    SECTION("multiple growth cycles")
    {
        portal::Buffer buffer = portal::Buffer::allocate(4);
        portal::BufferStreamWriter writer(buffer);

        for (int i = 0; i < 100; ++i)
        {
            const uint8_t value = static_cast<uint8_t>(i);
            writer.write(reinterpret_cast<const char*>(&value), sizeof(value));
        }

        REQUIRE(writer.size() == 100);
        REQUIRE(writer.capacity() >= 100);

        const portal::Buffer result = writer.get_buffer();
        for (int i = 0; i < 100; ++i)
        {
            REQUIRE(result[i] == static_cast<uint8_t>(i));
        }
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

    SECTION("stream operator writes and grows")
    {
        portal::Buffer buffer = portal::Buffer::allocate(5);
        portal::BufferStreamWriter writer(buffer);
        writer << "Hello, World!";

        REQUIRE(writer.size() >= 13);
        REQUIRE(writer.capacity() >= 13);

        const portal::Buffer result = writer.get_buffer();
        const std::string str = std::string(result.as<const char*>(), result.size);
        REQUIRE(str == "Hello, World!");
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
        REQUIRE(buffer.data == nullptr);
        uint8_t value = 123;
        writer.write(reinterpret_cast<char*>(&value), sizeof(value));
        REQUIRE(buffer.data != nullptr);
        REQUIRE(buffer.size == writer.INITIAL_CAPACITY);
    }
}

TEST_CASE("BufferStream write and read integration", "[buffer_stream]")
{
    SECTION("write then read")
    {
        portal::Buffer buffer;
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
