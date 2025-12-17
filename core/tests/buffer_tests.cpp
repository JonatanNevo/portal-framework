//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <array>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "portal/core/buffer.h"

using namespace testing;

#define EXPECT_BUFFER(buffer, ...) EXPECT_THAT(std::vector(buffer.as<uint8_t*>(), buffer.as<uint8_t*>() + buffer.size), __VA_ARGS__)

TEST(BufferTests, EmptyBuffer)
{
    {
        const portal::Buffer buffer{};

        EXPECT_EQ(buffer.size, 0);
        EXPECT_EQ(buffer.data, nullptr);
        EXPECT_FALSE(buffer.is_allocated());
    }

    {
        const portal::Buffer buffer;

        EXPECT_EQ(buffer.size, 0);
        EXPECT_EQ(buffer.data, nullptr);
        EXPECT_FALSE(buffer.is_allocated());
    }

    {
        const portal::Buffer buffer = nullptr;

        EXPECT_EQ(buffer.size, 0);
        EXPECT_EQ(buffer.data, nullptr);
        EXPECT_FALSE(buffer.is_allocated());
    }
}

TEST(BufferTests, BufferWithData)
{
    std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    const portal::Buffer buffer(data.data(), data.size());

    EXPECT_EQ(buffer.data, data.data());
    EXPECT_EQ(buffer.size, data.size());
    EXPECT_FALSE(buffer.is_allocated());
}

TEST(BufferTests, CopyConstructor)
{
    std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const portal::Buffer buffer(data.data(), data.size());

    {
        const portal::Buffer full_copy(buffer, buffer.size);
        EXPECT_EQ(full_copy.size, buffer.size);
        EXPECT_EQ(full_copy.data, buffer.data);
        EXPECT_BUFFER(full_copy, ElementsAreArray(data));
        EXPECT_FALSE(buffer.is_allocated());
    }

    {
        const portal::Buffer half_copy(buffer, 5);
        EXPECT_EQ(half_copy.size, 5);
        EXPECT_EQ(half_copy.data, buffer.data);
        EXPECT_BUFFER(half_copy, ElementsAreArray(std::span{data.begin(), data.begin() + 5}));
        EXPECT_FALSE(buffer.is_allocated());
    }
}

TEST(BufferTests, AllocationCopy)
{
    const std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const portal::Buffer buffer(data.data(), data.size());

    {
        const auto full_copy = portal::Buffer::copy(buffer);
        EXPECT_EQ(full_copy.size, buffer.size);
        EXPECT_NE(buffer.data, full_copy.data);
        EXPECT_BUFFER(full_copy, ElementsAreArray(data));
        EXPECT_TRUE(full_copy.is_allocated());
    }

    {
        const auto half_copy = portal::Buffer::copy(buffer.data, 5);
        EXPECT_EQ(half_copy.size, 5);
        EXPECT_NE(buffer.data, half_copy.data);
        EXPECT_BUFFER(half_copy, ElementsAreArray(std::span{data.begin(), data.begin() + 5}));
        EXPECT_TRUE(half_copy.is_allocated());
    }
}

TEST(BufferTests, Allocation)
{
    portal::Buffer buffer = portal::Buffer::allocate(10);

    EXPECT_EQ(buffer.size, 10);
    EXPECT_NE(buffer.data, nullptr);
    EXPECT_TRUE(buffer.is_allocated());

    for (int i = 0; i < 10; ++i)
    {
        buffer.as<uint8_t*>()[i] = i;
    }
    EXPECT_BUFFER(buffer, ElementsAre(0,1,2,3,4,5,6,7,8,9));

    buffer.release();
    EXPECT_EQ(buffer.size, 0);
    EXPECT_EQ(buffer.data, nullptr);
    EXPECT_FALSE(buffer.is_allocated());
}

TEST(BufferTests, EmptyAllocation)
{
    portal::Buffer buffer = portal::Buffer::allocate(0);
    EXPECT_EQ(buffer.size, 0);
    EXPECT_EQ(buffer.data, nullptr);
    EXPECT_FALSE(buffer.is_allocated());

    buffer.release();
    EXPECT_EQ(buffer.size, 0);
    EXPECT_EQ(buffer.data, nullptr);
    EXPECT_FALSE(buffer.is_allocated());
}

TEST(BufferTests, ZeroInitialize)
{
    portal::Buffer buffer = portal::Buffer::allocate(10);

    buffer.zero_initialize();
    EXPECT_BUFFER(buffer, Each(Eq(0)));
}

TEST(BufferTests, Write)
{
    const std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    portal::Buffer buffer = portal::Buffer::allocate(10);

    buffer.write(data.data(), data.size(), 0);
    EXPECT_BUFFER(buffer, ElementsAreArray(data));
}

TEST(BufferTests, WriteOffset)
{
    const std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    portal::Buffer buffer = portal::Buffer::allocate(20);

    buffer.write(data.data(), data.size(), 0);
    buffer.write(data.data(), data.size(), 10);
    EXPECT_BUFFER(buffer, ElementsAreArray({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
}

TEST(BufferTests, WriteAtBoundaries)
{
    portal::Buffer buffer = portal::Buffer::allocate(10);

    constexpr uint8_t value = 42;
    buffer.write(&value, 1, 0); // Write at beginning
    buffer.write(&value, 1, 9); // Write at end

    EXPECT_EQ(buffer[0], 42);
    EXPECT_EQ(buffer[9], 42);
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

    constexpr std::array data_array = {
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

    buffer = portal::Buffer::allocate(10);
    EXPECT_TRUE(static_cast<bool>(buffer));

    buffer.zero_initialize();
    EXPECT_TRUE(static_cast<bool>(buffer));

    buffer.release();
    EXPECT_FALSE(static_cast<bool>(buffer));
}

TEST(BufferTests, OperatorIndex)
{
    const std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
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

    constexpr Vec3 vector{1.0f, 2.0f, 3.0f};
    const portal::Buffer buffer(&vector, sizeof(Vec3));

    const Vec3* ptr = buffer.as<Vec3*>();
    EXPECT_FLOAT_EQ(ptr->x, 1.0f);
    EXPECT_FLOAT_EQ(ptr->y, 2.0f);
    EXPECT_FLOAT_EQ(ptr->z, 3.0f);
}

TEST(BufferTests, ZeroSizedOperations)
{
    portal::Buffer buffer = portal::Buffer::allocate(0);

    EXPECT_EQ(buffer.size, 0);
    EXPECT_EQ(buffer.data, nullptr);
    EXPECT_FALSE(buffer.is_allocated());
    EXPECT_FALSE(static_cast<bool>(buffer));

    buffer.zero_initialize(); // Should handle null data gracefully

    buffer.release();
    EXPECT_EQ(buffer.size, 0);
    EXPECT_EQ(buffer.data, nullptr);
    EXPECT_FALSE(buffer.is_allocated());
}

TEST(BufferTests, BufferAlignment)
{
    struct alignas(16) AlignedStruct
    {
        double value;
        int data[2];
    };

    portal::Buffer buffer = portal::Buffer::allocate(sizeof(AlignedStruct));

    // Write and read the aligned struct
    constexpr AlignedStruct test{3.14159, {42, 24}};
    buffer.write(&test, sizeof(AlignedStruct), 0);

    const auto& read_struct = buffer.read<AlignedStruct>();
    EXPECT_DOUBLE_EQ(read_struct.value, 3.14159);
    EXPECT_EQ(read_struct.data[0], 42);
    EXPECT_EQ(read_struct.data[1], 24);
}

TEST(BufferTests, OverlappingMemoryWrite)
{
    portal::Buffer buffer = portal::Buffer::allocate(10);

    for (int i = 0; i < 10; i++)
        buffer[i] = i + 1;
    EXPECT_BUFFER(buffer, ElementsAre(1, 2, 3, 4, 5, 6, 7, 8, 9, 10));

    const portal::Buffer temp_buffer = portal::Buffer::copy(buffer.data, 5);
    buffer.write(temp_buffer.data, 5, 2);

    EXPECT_BUFFER(buffer, ElementsAre(1, 2, 1, 2, 3, 4, 5, 8, 9, 10));
}

TEST(BufferTests, ModifyViaRead)
{
    portal::Buffer buffer = portal::Buffer::allocate(sizeof(int));
    buffer.zero_initialize();

    buffer.read<int>() = 42;
    EXPECT_EQ(buffer.read<int>(), 42);
}

TEST(BufferTests, WriteEdgeCases)
{
    portal::Buffer buffer = portal::Buffer::allocate(10);
    buffer.zero_initialize();

    buffer.write(nullptr, 0, 5);
    buffer.write(&buffer[0], 0, 0);
}
