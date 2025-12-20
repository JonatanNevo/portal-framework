//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <catch2/catch_test_macros.hpp>

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

TEST_CASE("PoolAllocator Basic Operations", "[memory][pool_allocator]")
{
    portal::PoolAllocator<TestObject, 10> allocator;

    SECTION("BasicAllocation")
    {
        const auto obj = allocator.alloc(42);
        REQUIRE(obj != nullptr);
        REQUIRE(obj->get_value() == 42);
        allocator.free(obj);
    }

    SECTION("FullCapacity")
    {
        std::vector<TestObject*> objects;

        // Fill pool to capacity
        for (int i = 0; i < 10; ++i)
        {
            objects.push_back(allocator.alloc(i));
            REQUIRE(objects[i]->get_value() == i);
        }

        // Next allocation should throw
        REQUIRE_THROWS_AS(allocator.alloc(999), std::bad_alloc);

        // Clean up
        for (const auto obj : objects)
        {
            allocator.free(obj);
        }
    }

    SECTION("MemoryReuse")
    {
        const auto obj1 = allocator.alloc(42);
        const auto addr1 = static_cast<void*>(obj1);
        allocator.free(obj1);

        const auto obj2 = allocator.alloc(24);
        const auto addr2 = static_cast<void*>(obj2);

        // Should reuse the same memory address
        REQUIRE(addr1 == addr2);
        REQUIRE(obj2->get_value() == 24);

        allocator.free(obj2);
    }

    SECTION("FreeNullptr")
    {
        // Should not crash
        REQUIRE_NOTHROW(allocator.free(nullptr));
    }

    SECTION("ClearPool")
    {
        // Fill the pool
        std::vector<TestObject*> objects;
        for (int i = 0; i < 10; ++i)
        {
            objects.push_back(allocator.alloc(i));
        }

        // Should throw when full
        REQUIRE_THROWS_AS(allocator.alloc(999), std::bad_alloc);

        // Clear without freeing individual objects
        allocator.clear();

        // Should be able to allocate again
        const auto obj = allocator.alloc(42);
        REQUIRE(obj != nullptr);
        REQUIRE(obj->get_value() == 42);
        allocator.free(obj);
    }
}

TEST_CASE("PoolAllocator Thread Safety", "[memory][pool_allocator]")
{
    portal::PoolAllocator<TestObject, 10> allocator;
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
    REQUIRE_NOTHROW(allocator.alloc(42));
}

TEST_CASE("PoolAllocator Custom Lock Type", "[memory][pool_allocator]")
{
    portal::PoolAllocator<TestObject, 5, std::mutex> custom_allocator;

    const auto obj = custom_allocator.alloc(42);
    REQUIRE(obj != nullptr);
    custom_allocator.free(obj);
}