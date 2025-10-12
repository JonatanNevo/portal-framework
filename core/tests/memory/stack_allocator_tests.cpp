//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <gtest/gtest.h>

#include "portal/core/memory/stack_allocator.h"

struct TestData
{
    int value;
    float ratio;
};

class StackAllocatorTest : public ::testing::Test
{
protected:
    portal::StackAllocator allocator{1024}; // 1KB stack
};

TEST_F(StackAllocatorTest, BasicAllocation)
{
    constexpr size_t size = sizeof(TestData);
    auto* data = static_cast<TestData*>(allocator.alloc(size));

    ASSERT_NE(nullptr, data);
    data->value = 42;
    data->ratio = 3.14f;

    EXPECT_EQ(42, data->value);
    EXPECT_FLOAT_EQ(3.14f, data->ratio);

    allocator.free(data);
}

TEST_F(StackAllocatorTest, TemplatedAllocation)
{
    auto* data = allocator.alloc<TestData>(42, 3.14f);
    ASSERT_NE(nullptr, data);
    EXPECT_EQ(42, data->value);
    EXPECT_FLOAT_EQ(3.14f, data->ratio);
    allocator.free(data);
}

TEST_F(StackAllocatorTest, MultipleAllocations)
{
    std::vector<int*> allocations;
    for (int i = 0; i < 10; ++i)
    {
        auto* ptr = allocator.alloc<int>(i);
        ASSERT_NE(nullptr, ptr);
        allocations.push_back(ptr);
    }

    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(i, *allocations[i]);
    }

    for (auto it = allocations.rbegin(); it != allocations.rend(); ++it)
    {
        allocator.free(*it);
    }
}

TEST_F(StackAllocatorTest, MarkerFunctionality)
{
    auto* first = allocator.alloc<int>(1);
    const portal::StackAllocator::marker marker = allocator.get_marker();

    [[maybe_unused]] auto* second = allocator.alloc<int>(2);
    [[maybe_unused]] auto* third = allocator.alloc<int>(3);

    allocator.free_to_marker(marker);
    EXPECT_EQ(1, *first);

    auto* new_alloc = allocator.alloc<int>(42);
    ASSERT_NE(nullptr, new_alloc);
    EXPECT_EQ(42, *new_alloc);

    allocator.free(new_alloc);
    allocator.free(first);
}

TEST_F(StackAllocatorTest, ClearFunctionality)
{
    std::vector<void*> allocations;
    for (int i = 0; i < 10; ++i)
    {
        allocations.push_back(allocator.alloc(sizeof(int)));
    }

    allocator.clear();
    auto* ptr = static_cast<int*>(allocator.alloc(sizeof(int) * 10));
    ASSERT_NE(nullptr, ptr);
    allocator.free(ptr);
}

TEST_F(StackAllocatorTest, OutOfMemory)
{
    EXPECT_THROW(allocator.alloc(2048), std::bad_alloc);
    auto* ptr = allocator.alloc<int>(42);
    ASSERT_NE(nullptr, ptr);
    allocator.free(ptr);
}

// Tests for BufferedAllocators
class DoubleBufferedAllocatorTest : public ::testing::Test
{
protected:
    portal::DoubleBufferedAllocator allocator{1024};
};

TEST_F(DoubleBufferedAllocatorTest, BasicFunctionality)
{
    auto* first = allocator.alloc<int>(42);
    EXPECT_EQ(42, *first);

    allocator.swap_buffers();
    auto* second = allocator.alloc<int>(24);
    EXPECT_EQ(24, *second);
    EXPECT_EQ(42, *first); // Original value preserved

    allocator.swap_buffers();
    auto* third = allocator.alloc<int>(99);
    EXPECT_EQ(99, *third);
    // Memory override
    EXPECT_EQ(99, *first);

    allocator.free(third);
    // Double free
    EXPECT_THROW(allocator.free(first), std::invalid_argument);
}

TEST_F(DoubleBufferedAllocatorTest, SwapAndClear)
{
    const auto* first = allocator.alloc<int>(42);
    allocator.swap_buffers();
    [[maybe_unused]] auto* second = allocator.alloc<int>(24);

    portal::StackAllocator& current = allocator.get_current_allocator();
    const auto marker = current.get_marker();
    [[maybe_unused]] auto* third = allocator.alloc<int>(99);
    current.free_to_marker(marker);

    EXPECT_EQ(42, *first);
    allocator.clear(0);
    allocator.swap_buffers();
    auto* fourth = allocator.alloc<int>(100);
    EXPECT_EQ(100, *fourth);
    allocator.free(fourth);
}

class TripleBufferedAllocatorTest : public ::testing::Test
{
protected:
    portal::BufferedAllocator<3> allocator{1024};
};

TEST_F(TripleBufferedAllocatorTest, ThreeBufferCycle)
{
    auto* a = allocator.alloc<TestData>(1, 1.1f);
    EXPECT_EQ(1, a->value);

    allocator.swap_buffers(); // Buffer 1
    auto* b = allocator.alloc<TestData>(2, 2.2f);
    EXPECT_EQ(2, b->value);

    allocator.swap_buffers(); // Buffer 2
    auto* c = allocator.alloc<TestData>(3, 3.3f);
    EXPECT_EQ(3, c->value);

    allocator.swap_buffers(); // Back to buffer 0
    // Buffer 0 was cleared on swap, so a is invalid
    auto* d = allocator.alloc<TestData>(4, 4.4f);
    EXPECT_EQ(4, d->value);
    EXPECT_EQ(2, b->value); // Still valid in buffer 1
    EXPECT_EQ(3, c->value); // Still valid in buffer 2

    allocator.free(d);
}

TEST_F(TripleBufferedAllocatorTest, GetAllocators)
{
    allocator.alloc<int>(1); // In buffer 0

    auto& current = allocator.get_current_allocator();
    auto& buffer0 = allocator.get_allocator(0);
    EXPECT_EQ(&current, &buffer0);

    allocator.swap_buffers();
    auto& newCurrent = allocator.get_current_allocator();
    auto& buffer1 = allocator.get_allocator(1);
    EXPECT_EQ(&newCurrent, &buffer1);
    EXPECT_NE(&newCurrent, &buffer0);
}

TEST_F(DoubleBufferedAllocatorTest, TemplatedFree)
{
    auto* obj = allocator.alloc<TestData>(42, 3.14f);
    ASSERT_NE(nullptr, obj);
    EXPECT_EQ(42, obj->value);

    // Should call destructor and free memory
    allocator.free(obj);

    // Should handle nullptr gracefully
    allocator.free<TestData>(nullptr);
}
