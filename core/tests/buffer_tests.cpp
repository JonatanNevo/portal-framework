//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <array>
#include <span>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "portal/core/buffer.h"

#define REQUIRE_BUFFER(buffer, ...) REQUIRE_THAT(std::vector(buffer.as<int8_t*>(), buffer.as<int8_t*>() + buffer.size), __VA_ARGS__)

using namespace Catch::Matchers;

TEST_CASE("Buffer Initialization", "[buffer]")
{
    SECTION("Empty")
    {
        const portal::Buffer buffer{};
        REQUIRE(buffer.size == 0);
        REQUIRE(buffer.data == nullptr);
        REQUIRE_FALSE(buffer.is_allocated());
    }

    SECTION("Default")
    {
        const portal::Buffer buffer;

        REQUIRE(buffer.size == 0);
        REQUIRE(buffer.data == nullptr);
        REQUIRE_FALSE(buffer.is_allocated());
    }

    SECTION("Nullptr")
    {
        const portal::Buffer buffer = nullptr;

        REQUIRE(buffer.size == 0);
        REQUIRE(buffer.data == nullptr);
        REQUIRE_FALSE(buffer.is_allocated());
    }

    SECTION("From vector")
    {
        std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        const portal::Buffer buffer(data.data(), data.size());

        REQUIRE(buffer.data == data.data());
        REQUIRE(buffer.size == data.size());
        REQUIRE_FALSE(buffer.is_allocated());
    }

    SECTION("Copy Constructor")
    {
        std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        const portal::Buffer buffer(data.data(), data.size());

        SECTION("Full Copy")
        {
            const portal::Buffer full_copy(buffer, buffer.size);
            REQUIRE(full_copy.size == buffer.size);
            REQUIRE(full_copy.data == buffer.data);

            REQUIRE_BUFFER(full_copy, RangeEquals(data));
            REQUIRE_FALSE(buffer.is_allocated());
        }

        SECTION("Partial Copy")
        {
            const portal::Buffer half_copy(buffer, 5);
            REQUIRE(half_copy.size == 5);
            REQUIRE(half_copy.data ==buffer.data);

            REQUIRE_BUFFER(half_copy, RangeEquals(std::span{data.begin(), data.begin() + 5}));
            REQUIRE_FALSE(buffer.is_allocated());
        }
    }

    SECTION("Allocation")
    {
        SECTION("Allocation And Release")
        {
            portal::Buffer buffer = portal::Buffer::allocate(10);

            REQUIRE(buffer.size == 10);
            REQUIRE_FALSE(buffer.data == nullptr);
            REQUIRE(buffer.is_allocated());

            for (int i = 0; i < 10; ++i)
            {
                buffer.as<uint8_t*>()[i] = i;
            }
            REQUIRE_BUFFER(buffer, RangeEquals({0,1,2,3,4,5,6,7,8,9}));

            buffer.release();
            REQUIRE(buffer.size == 0);
            REQUIRE(buffer.data == nullptr);
            REQUIRE_FALSE(buffer.is_allocated());
        }

        SECTION("From Buffer")
        {
            const std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            const portal::Buffer buffer(data.data(), data.size());

            SECTION("Full Copy")
            {
                const auto full_copy = portal::Buffer::copy(buffer);

                REQUIRE(full_copy.size == buffer.size);
                REQUIRE_FALSE(buffer.data == full_copy.data);

                REQUIRE_BUFFER(full_copy, RangeEquals(data));
                REQUIRE(full_copy.is_allocated());
            }

            SECTION("Partial Copy")
            {
                const auto half_copy = portal::Buffer::copy(buffer.data, 5);
                REQUIRE(half_copy.size == 5);
                REQUIRE_FALSE(buffer.data == half_copy.data);

                REQUIRE_BUFFER(half_copy, RangeEquals(std::span{data.begin(), data.begin() + 5}));
                REQUIRE(half_copy.is_allocated());
            }
        }

        SECTION("Empty")
        {
            portal::Buffer buffer = portal::Buffer::allocate(0);
            REQUIRE(buffer.size == 0);
            REQUIRE(buffer.data == nullptr);
            REQUIRE_FALSE(buffer.is_allocated());

            buffer.release();
            REQUIRE(buffer.size == 0);
            REQUIRE(buffer.data == nullptr);
            REQUIRE_FALSE(buffer.is_allocated());
        }

        SECTION("Zero Initialize")
        {
            portal::Buffer buffer = portal::Buffer::allocate(10);

            buffer.zero_initialize();
            REQUIRE_BUFFER(buffer, AllMatch(Predicate<int>([] (const auto& val){ return val == 0; })));
        }
    }
}

TEST_CASE("Buffer Operations", "[buffer]")
{
    SECTION("Write")
    {
        const std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        SECTION("Entire Array")
        {
            const portal::Buffer buffer = portal::Buffer::allocate(10);

            buffer.write(data.data(), data.size(), 0);
            REQUIRE_BUFFER(buffer, RangeEquals(data));
        }

        SECTION("With Offset")
        {
            const portal::Buffer buffer = portal::Buffer::allocate(20);

            buffer.write(data.data(), data.size(), 0);
            buffer.write(data.data(), data.size(), 10);
            REQUIRE_BUFFER(buffer, RangeEquals({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
        }

        SECTION("At Boundaries")
        {
            portal::Buffer buffer = portal::Buffer::allocate(10);

            constexpr uint8_t value = 42;
            buffer.write(&value, 1, 0); // Write at beginning
            buffer.write(&value, 1, 9); // Write at end

            REQUIRE(buffer[0] == 42);
            REQUIRE(buffer[9] == 42);
        }
    }

    SECTION("Read")
    {
        struct TestStruct
        {
            int a;
            float b;
            bool c;
        };

        constexpr std::array data_array = {
            TestStruct{10, 1.1f, false},
            TestStruct{20, 2.2f, true},
            TestStruct{30, 3.3f, false}
        };

        portal::Buffer buffer(data_array.data(), sizeof(data_array));

        const auto& first = buffer.read<TestStruct>(0);
        REQUIRE(first.a == 10);
        REQUIRE_THAT(first.b, WithinRel(1.1f));
        REQUIRE(first.c == false);

        const auto& second = buffer.read<TestStruct>(sizeof(TestStruct));
        REQUIRE(second.a == 20);
        REQUIRE_THAT(second.b, WithinRel(2.2f));
        REQUIRE(second.c == true);

        const auto& third = buffer.read<TestStruct>(2 * sizeof(TestStruct));
        REQUIRE(third.a == 30);
        REQUIRE_THAT(third.b, WithinRel(3.3f));
        REQUIRE(third.c == false);


        SECTION("Templated Offset")
        {
            REQUIRE(buffer.read<int>(0) == 10);
            REQUIRE_THAT(buffer.read<float>(sizeof(int)), WithinRel(1.1f));
            REQUIRE(buffer.read<bool>(sizeof(int) + sizeof(float)) == false);
        }

        SECTION("Const")
        {
            struct TestStructConst
            {
                int a;
                float b;
            };

            TestStructConst test_struct{123, 3.14f};
            const portal::Buffer buffer_const(reinterpret_cast<uint8_t*>(&test_struct), sizeof(test_struct));

            const auto& data = buffer_const.read<TestStruct>();
            REQUIRE(data.a == 123);
            REQUIRE_THAT(data.b, WithinRel(3.14f));
        }

        SECTION("Read Returns Reference")
        {
            portal::Buffer buffer_int = portal::Buffer::allocate(sizeof(int));
            buffer.zero_initialize();
            REQUIRE(buffer.read<int>() == 0);

            buffer.read<int>() = 42;
            REQUIRE(buffer.read<int>() == 42);
        }
    }

    SECTION("Operators")
    {
        SECTION("Bool")
        {
            portal::Buffer buffer;
            REQUIRE_FALSE(static_cast<bool>(buffer));

            buffer = portal::Buffer::allocate(10);
            REQUIRE(static_cast<bool>(buffer));

            buffer.zero_initialize();
            REQUIRE(static_cast<bool>(buffer));

            buffer.release();
            REQUIRE_FALSE(static_cast<bool>(buffer));
        }

        SECTION("Index")
        {
            const std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            const portal::Buffer buffer(data.data(), data.size());

            REQUIRE(buffer[0] == 1);
            REQUIRE(buffer[5] == 6);
            REQUIRE(buffer[9] == 10);
        }
    }

    SECTION("`as<T>`")
    {
        struct Vec3
        {
            float x, y, z;
        };

        constexpr Vec3 vector{1.0f, 2.0f, 3.0f};
        const portal::Buffer buffer(&vector, sizeof(Vec3));

        const Vec3* ptr = buffer.as<Vec3*>();
        REQUIRE_THAT(ptr->x, WithinRel(1.0f));
        REQUIRE_THAT(ptr->y, WithinRel(2.0f));
        REQUIRE_THAT(ptr->z, WithinRel(3.0f));
    }
}

TEST_CASE("Buffer Alignment", "[buffer]")
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
    REQUIRE_THAT(read_struct.value, WithinRel(3.14159));
    REQUIRE(read_struct.data[0] == 42);
    REQUIRE(read_struct.data[1] ==  24);
}

TEST_CASE("Buffer Edge Cases", "[buffer]")
{
    SECTION("Zero sized operations")
    {
        portal::Buffer buffer = portal::Buffer::allocate(0);

        REQUIRE(buffer.size == 0);
        REQUIRE(buffer.data == nullptr);
        REQUIRE_FALSE(buffer.is_allocated());
        REQUIRE_FALSE(static_cast<bool>(buffer));

        buffer.zero_initialize(); // Should handle null data gracefully

        buffer.release();
        REQUIRE(buffer.size == 0);
        REQUIRE(buffer.data == nullptr);
        REQUIRE_FALSE(buffer.is_allocated());
    }

    SECTION("Writing On Overlapping Memory")
    {
        portal::Buffer buffer = portal::Buffer::allocate(10);
        for (int i = 0; i < 10; i++)
            buffer[i] = i + 1;
        REQUIRE_BUFFER(buffer, RangeEquals({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));

        const portal::Buffer temp_buffer = portal::Buffer::copy(buffer.data, 5);
        buffer.write(temp_buffer.data, 5, 2);

        REQUIRE_BUFFER(buffer, RangeEquals({1, 2, 1, 2, 3, 4, 5, 8, 9, 10}));
    }

    SECTION("Write on null")
    {
        portal::Buffer buffer = portal::Buffer::allocate(10);
        buffer.zero_initialize();

        buffer.write(nullptr, 0, 5);
        buffer.write(&buffer[0], 0, 0);
    }
}
