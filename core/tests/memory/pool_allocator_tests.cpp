//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <gtest/gtest.h>

#include "portal/core/memory/pool_allocator.h"
#include "portal/core/memory/stack_allocator.h"

class TestObject
{
public:
    explicit TestObject(const int value = 0) :
        value(value) {}

    [[nodiscard]] uint64_t get_value() const { return value; }
    void set_value(const int value) { this->value = value; }

private:
    uint64_t value;
};

class PoolAllocatorTest : public ::testing::Test
{
protected:
    portal::PoolAllocator<TestObject, 10> allocator;
};

// Test basic allocation
TEST_F(PoolAllocatorTest, BasicAllocation)
{
    const auto obj = allocator.alloc(42);
    ASSERT_NE(nullptr, obj);
    EXPECT_EQ(42, obj->get_value());
    allocator.free(obj);
}

// Test allocating to full capacity
TEST_F(PoolAllocatorTest, FullCapacity)
{
    std::vector<TestObject*> objects;

    // Fill pool to capacity
    for (int i = 0; i < 10; ++i)
    {
        objects.push_back(allocator.alloc(i));
        EXPECT_EQ(i, objects[i]->get_value());
    }

    // Next allocation should throw
    EXPECT_THROW(allocator.alloc(999), std::bad_alloc);

    // Clean up
    for (const auto obj : objects)
    {
        allocator.free(obj);
    }
}

// Test memory reuse
TEST_F(PoolAllocatorTest, MemoryReuse)
{
    const auto obj1 = allocator.alloc(42);
    const auto addr1 = static_cast<void*>(obj1);
    allocator.free(obj1);

    const auto obj2 = allocator.alloc(24);
    const auto addr2 = static_cast<void*>(obj2);

    // Should reuse the same memory address
    EXPECT_EQ(addr1, addr2);
    EXPECT_EQ(24, obj2->get_value());

    allocator.free(obj2);
}

// Test freeing nullptr
TEST_F(PoolAllocatorTest, FreeNullptr)
{
    // Should not crash
    EXPECT_NO_THROW(allocator.free(nullptr));
}

// Test clear functionality
TEST_F(PoolAllocatorTest, ClearPool)
{
    // Fill the pool
    std::vector<TestObject*> objects;
    for (int i = 0; i < 10; ++i)
    {
        objects.push_back(allocator.alloc(i));
    }

    // Should throw when full
    EXPECT_THROW(allocator.alloc(999), std::bad_alloc);

    // Clear without freeing individual objects
    allocator.clear();

    // Should be able to allocate again
    const auto obj = allocator.alloc(42);
    ASSERT_NE(nullptr, obj);
    EXPECT_EQ(42, obj->get_value());
    allocator.free(obj);
}

// Test thread safety
TEST_F(PoolAllocatorTest, ThreadSafety)
{
    allocator.clear();
    constexpr int THREAD_COUNT = 5;
    std::atomic<int> success_count(0);
    std::vector<std::thread> threads;

    for (int t = 0; t < THREAD_COUNT; ++t)
    {
        threads.emplace_back(
            [&]()
            {
                std::vector<TestObject*> thread_objects;

                for (int i = 0; i < 5; ++i)
                {
                    try
                    {
                        auto obj = allocator.alloc(i);
                        thread_objects.push_back(obj);
                        ++success_count;
                    }
                    catch (std::bad_alloc&)
                    {
                        // Expected when pool is full
                    }
                    std::this_thread::yield();
                }

                for (const auto obj : thread_objects)
                {
                    allocator.free(obj);
                }
            }
        );
    }

    for (auto& t : threads)
    {
        t.join();
    }

    // Verify we can still allocate
    EXPECT_NO_THROW(allocator.alloc(42));
}

// Test custom lock type
TEST_F(PoolAllocatorTest, CustomLockType)
{
    portal::PoolAllocator<TestObject, 5, std::mutex> custom_allocator;

    const auto obj = custom_allocator.alloc(42);
    ASSERT_NE(nullptr, obj);
    custom_allocator.free(obj);
}
