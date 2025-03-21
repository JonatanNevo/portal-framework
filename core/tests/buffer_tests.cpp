//
// Created by thejo on 3/21/2025.
//

#include <array>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "portal/core/buffer.h"

using namespace testing;

#define EXPECT_BUFFER(buffer, ...) EXPECT_THAT(std::vector(static_cast<uint8_t*>(buffer.data), static_cast<uint8_t*>(buffer.data) + buffer.size), __VA_ARGS__)

TEST(BufferTests, EmptyBuffer)
{
    {
        const portal::Buffer buffer{};

        EXPECT_EQ(buffer.size, 0);
        EXPECT_EQ(buffer.data, nullptr);
    }

    {
        const portal::Buffer buffer;

        EXPECT_EQ(buffer.size, 0);
        EXPECT_EQ(buffer.data, nullptr);
    }
}

TEST(BufferTests, BufferWithData)
{
    std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    const portal::Buffer buffer(data.data(), data.size());

    EXPECT_EQ(buffer.data, data.data());
    EXPECT_EQ(buffer.size, data.size());
}

TEST(BufferTests, CopyConstructor)
{
    std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const portal::Buffer buffer(data.data(), data.size());

    {
        const portal::Buffer full_copy(buffer, buffer.size);
        EXPECT_EQ(full_copy.size, buffer.size);
        EXPECT_EQ(full_copy.data, buffer.data);
    }

    {
        const portal::Buffer half_copy(buffer, 5);
        EXPECT_EQ(half_copy.size, 5);
        EXPECT_EQ(half_copy.data, buffer.data);
    }
}

TEST(BufferTests, AllocationCopy)
{
    std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const portal::Buffer buffer(data.data(), data.size());

    {
        auto full_copy = portal::Buffer::copy(buffer);
        EXPECT_EQ(full_copy.size, buffer.size);
        EXPECT_NE(buffer.data, full_copy.data);
        EXPECT_BUFFER(full_copy, ElementsAreArray(data));
        full_copy.release();
    }

    {
        auto half_copy = portal::Buffer::copy(buffer.data, 5);
        EXPECT_EQ(half_copy.size, 5);
        EXPECT_NE(buffer.data, half_copy.data);
        EXPECT_BUFFER(half_copy, ElementsAre(1,2,3,4,5));
        half_copy.release();
    }
}

TEST(BufferTests, Allocation)
{
    portal::Buffer buffer;

    buffer.allocate(10);
    EXPECT_EQ(buffer.size, 10);
    EXPECT_NE(buffer.data, nullptr);

    for (int i = 0; i < 10; ++i)
    {
        static_cast<uint8_t*>(buffer.data)[i] = i;
    }
    EXPECT_BUFFER(buffer, ElementsAre(0,1,2,3,4,5,6,7,8,9));

    buffer.release();
    EXPECT_EQ(buffer.size, 0);
    EXPECT_EQ(buffer.data, nullptr);
}

TEST(BufferTests, Reallocation)
{
    portal::Buffer buffer;
    buffer.allocate(10);

    for (int i = 0; i < 10; ++i)
        static_cast<uint8_t*>(buffer.data)[i] = i + 1;

    buffer.allocate(10);
    EXPECT_EQ(buffer.size, 10);
    EXPECT_NE(buffer.data, nullptr);

    EXPECT_BUFFER(buffer, ElementsAre(Ne(1), Ne(2), Ne(3), Ne(4), Ne(5), Ne(6), Ne(7), Ne(8), Ne(9), Ne(10)));

    buffer.release();
}

TEST(BufferTests, EmptyAllocation)
{
    portal::Buffer buffer;

    buffer.allocate(0);
    EXPECT_EQ(buffer.size, 0);
    EXPECT_EQ(buffer.data, nullptr);

    buffer.release();
    EXPECT_EQ(buffer.size, 0);
    EXPECT_EQ(buffer.data, nullptr);
}

TEST(BufferTests, ZeroInitialize)
{
    portal::Buffer buffer;
    buffer.allocate(10);

    buffer.zero_initialize();
    EXPECT_BUFFER(buffer, Each(Eq(0)));

    buffer.release();
}

TEST(BufferTests, Write)
{
    const std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    portal::Buffer buffer;
    buffer.allocate(10);

    buffer.write(data.data(), data.size(), 0);
    EXPECT_BUFFER(buffer, ElementsAreArray(data));

    buffer.release();
}

TEST(BufferTests, WriteOffset)
{

    const std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    portal::Buffer buffer;
    buffer.allocate(20);

    buffer.write(data.data(), data.size(), 0);
    buffer.write(data.data(), data.size(), 10);
    EXPECT_BUFFER(buffer, ElementsAreArray({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));

    buffer.release();
}

TEST(BufferTests, WriteAtBoundaries)
{
    portal::Buffer buffer;
    buffer.allocate(10);

    constexpr uint8_t value = 42;
    buffer.write(&value, 1, 0);  // Write at beginning
    buffer.write(&value, 1, 9);  // Write at end

    EXPECT_EQ(buffer[0], 42);
    EXPECT_EQ(buffer[9], 42);

    buffer.release();
}


TEST(BufferTests, ReadTemplated)
{
    struct TestStruct
    {
        int a;
        float b;
        bool c;
    };

    TestStruct test_struct{
        .a = 123,
        .b = 3.14f,
        .c = true
    };

    portal::Buffer buffer(reinterpret_cast<uint8_t*>(&test_struct), sizeof(test_struct));

    const auto read_struct = buffer.read<TestStruct>();
    EXPECT_EQ(read_struct.a, test_struct.a);
    EXPECT_FLOAT_EQ(read_struct.b, test_struct.b);
    EXPECT_EQ(read_struct.c, test_struct.c);
}

TEST(BufferTests, ReadTemplatedOffset)
{
    struct Data
    {
        int a;
        float b;
        bool c;
    };

    std::array data_array = {
        Data{10, 1.1f, false},
        Data{20, 2.2f, true},
        Data{30, 3.3f, false}
    };

    portal::Buffer buffer(data_array.data(), sizeof(data_array));


    const auto& first = buffer.read<Data>(0);
    EXPECT_EQ(first.a, 10);
    EXPECT_FLOAT_EQ(first.b, 1.1f);
    EXPECT_EQ(first.c, false);

    const auto& second = buffer.read<Data>(sizeof(Data));
    EXPECT_EQ(second.a, 20);
    EXPECT_FLOAT_EQ(second.b, 2.2f);
    EXPECT_EQ(second.c, true);

    const auto& third = buffer.read<Data>(2 * sizeof(Data));
    EXPECT_EQ(third.a, 30);
    EXPECT_FLOAT_EQ(third.b, 3.3f);
    EXPECT_EQ(third.c, false);

    EXPECT_EQ(buffer.read<int>(0), 10);
    EXPECT_FLOAT_EQ(buffer.read<float>(sizeof(int)), 1.1f);
    EXPECT_EQ(buffer.read<bool>(sizeof(int) + sizeof(float)), false);
}

TEST(BufferTests, ReadBytes)
{
    std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const portal::Buffer buffer(data.data(), data.size());

    const auto read_data = buffer.read_bytes(5, 0);
    EXPECT_THAT(std::vector(read_data, read_data + 5), ElementsAre(1, 2, 3, 4, 5));
    delete[] read_data;
}

TEST(BufferTests, ConstRead)
{
    struct TestStruct
    {
        int a;
        float b;
    };

    TestStruct test_struct{123, 3.14f};
    const portal::Buffer buffer(reinterpret_cast<uint8_t*>(&test_struct), sizeof(test_struct));

    const auto& data = buffer.read<TestStruct>();
    EXPECT_EQ(data.a, 123);
    EXPECT_FLOAT_EQ(data.b, 3.14f);
}

TEST(BufferTests, OperatorBool)
{
    portal::Buffer buffer;
    EXPECT_FALSE(static_cast<bool>(buffer));

    buffer.allocate(10);
    EXPECT_TRUE(static_cast<bool>(buffer));

    buffer.zero_initialize();
    EXPECT_TRUE(static_cast<bool>(buffer));

    buffer.release();
    EXPECT_FALSE(static_cast<bool>(buffer));
}

TEST(BufferTests, OperatorIndex)
{
    std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const portal::Buffer buffer(data.data(), data.size());

    EXPECT_EQ(buffer[0], 1);
    EXPECT_EQ(buffer[5], 6);
    EXPECT_EQ(buffer[9], 10);
}

TEST(BufferTests, AsTemplated)
{
    struct Vec3
    {
        float x, y, z;
    };

    Vec3 vector{1.0f, 2.0f, 3.0f};
    const portal::Buffer buffer(&vector, sizeof(Vec3));

    const Vec3* ptr = buffer.as<Vec3>();
    EXPECT_FLOAT_EQ(ptr->x, 1.0f);
    EXPECT_FLOAT_EQ(ptr->y, 2.0f);
    EXPECT_FLOAT_EQ(ptr->z, 3.0f);
}

TEST(BufferTests, ZeroSizedOperations)
{
    portal::Buffer buffer;
    buffer.allocate(0);

    EXPECT_EQ(buffer.size, 0);
    EXPECT_EQ(buffer.data, nullptr);
    EXPECT_FALSE(static_cast<bool>(buffer));

    buffer.zero_initialize(); // Should handle null data gracefully

    buffer.release();
    EXPECT_EQ(buffer.size, 0);
    EXPECT_EQ(buffer.data, nullptr);
}

TEST(BufferTests, BufferAlignment)
{
    struct alignas(16) AlignedStruct
    {
        double value;
        int data[2];
    };

    portal::Buffer buffer;
    buffer.allocate(sizeof(AlignedStruct));

    // Write and read the aligned struct
    AlignedStruct test{3.14159, {42, 24}};
    buffer.write(&test, sizeof(AlignedStruct), 0);

    const auto& read_struct = buffer.read<AlignedStruct>();
    EXPECT_DOUBLE_EQ(read_struct.value, 3.14159);
    EXPECT_EQ(read_struct.data[0], 42);
    EXPECT_EQ(read_struct.data[1], 24);

    buffer.release();
}

TEST(BufferTests, OverlappingMemoryWrite)
{
    portal::Buffer buffer;
    buffer.allocate(10);

    for (int i = 0; i < 10; i++)
        buffer[i] = i + 1;

    buffer.write(buffer.data, 5, 2);

    EXPECT_BUFFER(buffer, ElementsAre(1, 2, 1, 2, 3, 4, 5, 8, 9, 10));
    buffer.release();
}

TEST(BufferTests, ModifyViaRead)
{
    portal::Buffer buffer;
    buffer.allocate(sizeof(int));
    buffer.zero_initialize();

    buffer.read<int>() = 42;
    EXPECT_EQ(buffer.read<int>(), 42);

    buffer.release();
}

TEST(BufferTests, GetSize)
{
    portal::Buffer buffer;
    EXPECT_EQ(buffer.get_size(), 0);

    buffer.allocate(42);
    EXPECT_EQ(buffer.get_size(), 42);

    buffer.release();
}

TEST(BufferTests, WriteEdgeCases)
{
    portal::Buffer buffer;
    buffer.allocate(10);
    buffer.zero_initialize();

    buffer.write(nullptr, 0, 5);
    buffer.write(&buffer[0], 0, 0);

    buffer.release();
}
