//
// Created by Jonatan Nevo on 22/03/2025.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "portal/core/buffer_stream.h"

TEST(BufferStreamTests, ReaderTest)
{
    std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8};
    const portal::Buffer buffer(data.data(), data.size());

    portal::BufferStreamReader reader(buffer);
    EXPECT_EQ(reader.position(), 0);

    uint8_t value;
    reader.read(reinterpret_cast<char*>(&value), sizeof(value));
    EXPECT_EQ(value, 1);
    EXPECT_EQ(reader.position(), 1);

    reader.read(reinterpret_cast<char*>(&value), sizeof(value));
    EXPECT_EQ(value, 2);
    EXPECT_EQ(reader.position(), 2);
}

TEST(BufferStreamTests, ReadMultipleBytes)
{
    std::vector<uint8_t> data{10, 20, 30, 40, 50};
    const portal::Buffer buffer(data.data(), data.size());

    portal::BufferStreamReader reader(buffer);

    std::vector<uint8_t> readBuffer(3);
    reader.read(reinterpret_cast<char*>(readBuffer.data()), 3);

    EXPECT_EQ(readBuffer[0], 10);
    EXPECT_EQ(readBuffer[1], 20);
    EXPECT_EQ(readBuffer[2], 30);
    EXPECT_EQ(reader.position(), 3);
}

TEST(BufferStreamTests, SeekInReader)
{
    std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8};
    const portal::Buffer buffer(data.data(), data.size());

    portal::BufferStreamReader reader(buffer);

    reader.seekg(3, std::ios::beg);
    EXPECT_EQ(reader.position(), 3);

    uint8_t value;
    reader.read(reinterpret_cast<char*>(&value), sizeof(value));
    EXPECT_EQ(value, 4);

    reader.seekg(-2, std::ios::cur);
    reader.read(reinterpret_cast<char*>(&value), sizeof(value));
    EXPECT_EQ(value, 3);

    reader.seekg(-2, std::ios::end);
    reader.read(reinterpret_cast<char*>(&value), sizeof(value));
    EXPECT_EQ(value, 7);
}

TEST(BufferStreamTests, ReadComplexType)
{
    struct TestStruct {
        int a;
        float b;
        char c;
    };

    TestStruct original{42, 3.14f, 'X'};
    const portal::Buffer buffer(&original, sizeof(TestStruct));

    portal::BufferStreamReader reader(buffer);

    TestStruct result{};
    reader.read(reinterpret_cast<char*>(&result), sizeof(TestStruct));

    EXPECT_EQ(result.a, 42);
    EXPECT_FLOAT_EQ(result.b, 3.14f);
    EXPECT_EQ(result.c, 'X');
}

TEST(BufferStreamTests, ReaderZeroBytesTest) {
    std::vector<uint8_t> data{1, 2, 3};
    const portal::Buffer buffer(data.data(), data.size());
    portal::BufferStreamReader reader(buffer);

    std::vector<uint8_t> read_buffer(3);
    reader.read(reinterpret_cast<char*>(read_buffer.data()), 0);
    EXPECT_EQ(reader.position(), 0);
}

TEST(BufferStreamTests, WriterTest)
{
    portal::Buffer buffer;
    buffer.allocate(2);

    portal::BufferStreamWriter writer(buffer);
    EXPECT_EQ(writer.size(), 0);
    EXPECT_FALSE(writer.full());

    uint8_t value = 42;
    writer.write(reinterpret_cast<const char*>(&value), sizeof(value));
    EXPECT_EQ(writer.size(), 1);
    EXPECT_EQ(buffer[0], 42);

    value = 123;
    writer.write(reinterpret_cast<const char*>(&value), sizeof(value));
    EXPECT_EQ(writer.size(), 2);
    EXPECT_EQ(buffer[1], 123);
    EXPECT_TRUE(writer.full());

    buffer.release();
}

TEST(BufferStreamTests, WriteMultipleBytes)
{
    portal::Buffer buffer;
    buffer.allocate(3);

    portal::BufferStreamWriter writer(buffer);

    const std::vector<uint8_t> data{10, 20, 30};
    writer.write(reinterpret_cast<const char*>(data.data()), data.size());

    EXPECT_EQ(buffer[0], 10);
    EXPECT_EQ(buffer[1], 20);
    EXPECT_EQ(buffer[2], 30);
    EXPECT_EQ(writer.size(), 3);
    EXPECT_TRUE(writer.full());

    buffer.release();
}

TEST(BufferStreamTests, WriteComplexType)
{
    struct TestStruct {
        int a;
        float b;
        char c;
    };

    const TestStruct data{42, 3.14f, 'X'};

    portal::Buffer buffer;
    buffer.allocate(sizeof(TestStruct));

    portal::BufferStreamWriter writer(buffer);
    writer.write(reinterpret_cast<const char*>(&data), sizeof(TestStruct));

    const TestStruct* result = buffer.as<TestStruct>();
    EXPECT_EQ(result->a, 42);
    EXPECT_FLOAT_EQ(result->b, 3.14f);
    EXPECT_EQ(result->c, 'X');

    buffer.release();
}

TEST(BufferStreamTests, WriterFullCondition)
{
    portal::Buffer buffer;
    buffer.allocate(2);

    portal::BufferStreamWriter writer(buffer);

    const std::vector<uint8_t> data{10, 20};
    writer.write(reinterpret_cast<const char*>(data.data()), data.size());

    EXPECT_TRUE(writer.full());

    uint8_t extra = 30;
    writer.write(reinterpret_cast<const char*>(&extra), sizeof(extra));
    EXPECT_EQ(writer.size(), 2);
    EXPECT_EQ(buffer[0], 10);
    EXPECT_EQ(buffer[1], 20);

    buffer.release();
}

TEST(BufferStreamTests, EmptyBufferOperations)
{
    portal::Buffer buffer;

    portal::BufferStreamReader reader(buffer);
    uint8_t value = 123;
    reader.read(reinterpret_cast<char*>(&value), sizeof(value));
    EXPECT_EQ(value, 123);

    portal::BufferStreamWriter writer(buffer);
    writer.write(reinterpret_cast<const char*>(&value), sizeof(value));
    EXPECT_EQ(writer.size(), 0);
}


TEST(BufferStreamTests, GetBufferFromWriter)
{
    portal::Buffer buffer;
    buffer.allocate(10);

    portal::BufferStreamWriter writer(buffer);
    const std::vector<uint8_t> data{1, 2, 3, 4};
    writer.write(reinterpret_cast<const char*>(data.data()), data.size());

    const portal::Buffer result_buffer = writer.get_buffer();
    EXPECT_EQ(result_buffer.get_size(), 4);

    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_EQ(result_buffer[i], data[i]);
    }

    buffer.release();
}

TEST(BufferStreamTests, ReadBeyondEnd) {
    std::vector<uint8_t> data{1, 2, 3};
    const portal::Buffer buffer(data.data(), data.size());

    portal::BufferStreamReader reader(buffer);
    std::vector<uint8_t> read_buffer(5, 0);

    reader.read(reinterpret_cast<char*>(read_buffer.data()), 5);

    EXPECT_EQ(read_buffer[0], 1);
    EXPECT_EQ(read_buffer[1], 2);
    EXPECT_EQ(read_buffer[2], 3);
    EXPECT_TRUE(reader.eof());
}

TEST(BufferStreamTests, InvalidSeekOperations) {
    std::vector<uint8_t> data{1, 2, 3, 4};
    const portal::Buffer buffer(data.data(), data.size());

    portal::BufferStreamReader reader(buffer);

    reader.seekg(10, std::ios::beg);
    EXPECT_TRUE(reader.fail());
    reader.clear();

    reader.seekg(-2, std::ios::beg);
    EXPECT_TRUE(reader.fail());
    reader.clear();

    reader.seekg(2, std::ios::beg);
    EXPECT_EQ(reader.position(), 2);
}

TEST(BufferStreamTests, WriteAndReadIntegration) {
    portal::Buffer buffer;
    buffer.allocate(10);

    {
        portal::BufferStreamWriter writer(buffer);
        const std::vector<uint8_t> data{10, 20, 30, 40};
        writer.write(reinterpret_cast<const char*>(data.data()), data.size());
    }

    {
        portal::BufferStreamReader reader(buffer);
        std::vector<uint8_t> readData(4);
        reader.read(reinterpret_cast<char*>(readData.data()), 4);

        EXPECT_EQ(readData[0], 10);
        EXPECT_EQ(readData[1], 20);
        EXPECT_EQ(readData[2], 30);
        EXPECT_EQ(readData[3], 40);
    }

    buffer.release();
}
