//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "portal/core/memory/stack_allocator.h"

using namespace Catch::Matchers;

struct TestData
{
    int value;
    float ratio;
};

TEST_CASE("StackAllocator Basic Operations", "[memory][stack_allocator]")
{
    portal::StackAllocator allocator{1024}; // 1KB stack

    SECTION("BasicAllocation")
    {
        constexpr size_t size = sizeof(TestData);
        auto* data = static_cast<TestData*>(allocator.alloc(size));

        REQUIRE(data != nullptr);
        data->value = 42;
        data->ratio = 3.14f;

        REQUIRE(data->value == 42);
        REQUIRE_THAT(data->ratio, WithinRel(3.14f));

        allocator.free(data);
    }

    SECTION("TemplatedAllocation")
    {
        auto* data = allocator.alloc<TestData>(42, 3.14f);
        REQUIRE(data != nullptr);
        REQUIRE(data->value == 42);
        REQUIRE_THAT(data->ratio, WithinRel(3.14f));
        allocator.free(data);
    }

    SECTION("MultipleAllocations")
    {
        std::vector<int*> allocations;
        for (int i = 0; i < 10; ++i)
        {
            auto* ptr = allocator.alloc<int>(i);
            REQUIRE(ptr != nullptr);
            allocations.push_back(ptr);
        }

        for (int i = 0; i < 10; ++i)
        {
            REQUIRE(*allocations[i] == i);
        }

        for (auto it = allocations.rbegin(); it != allocations.rend(); ++it)
        {
            allocator.free(*it);
        }
    }

    SECTION("MarkerFunctionality")
    {
        auto* first = allocator.alloc<int>(1);
        const portal::StackAllocator::marker marker = allocator.get_marker();

        [[maybe_unused]] auto* second = allocator.alloc<int>(2);
        [[maybe_unused]] auto* third = allocator.alloc<int>(3);

        allocator.free_to_marker(marker);
        REQUIRE(*first == 1);

        auto* new_alloc = allocator.alloc<int>(42);
        REQUIRE(new_alloc != nullptr);
        REQUIRE(*new_alloc == 42);

        allocator.free(new_alloc);
        allocator.free(first);
    }

    SECTION("ClearFunctionality")
    {
        std::vector<void*> allocations;
        for (int i = 0; i < 10; ++i)
        {
            allocations.push_back(allocator.alloc(sizeof(int)));
        }

        allocator.clear();
        auto* ptr = static_cast<int*>(allocator.alloc(sizeof(int) * 10));
        REQUIRE(ptr != nullptr);
        allocator.free(ptr);
    }

    SECTION("OutOfMemory")
    {
        REQUIRE_THROWS_AS(allocator.alloc(2048), std::bad_alloc);
        auto* ptr = allocator.alloc<int>(42);
        REQUIRE(ptr != nullptr);
        allocator.free(ptr);
    }
}

TEST_CASE("DoubleBufferedAllocator", "[memory][stack_allocator]")
{
    portal::DoubleBufferedAllocator allocator{1024};

    SECTION("BasicFunctionality")
    {
        auto* first = allocator.alloc<int>(42);
        REQUIRE(*first == 42);

        allocator.swap_buffers();
        auto* second = allocator.alloc<int>(24);
        REQUIRE(*second == 24);
        REQUIRE(*first == 42); // Original value preserved

        allocator.swap_buffers();
        auto* third = allocator.alloc<int>(99);
        REQUIRE(*third == 99);
        // Memory override
        REQUIRE(*first == 99);

        allocator.free(third);
        // Double free
        REQUIRE_THROWS_AS(allocator.free(first), std::invalid_argument);
    }

    SECTION("SwapAndClear")
    {
        const auto* first = allocator.alloc<int>(42);
        allocator.swap_buffers();
        [[maybe_unused]] auto* second = allocator.alloc<int>(24);

        portal::StackAllocator& current = allocator.get_current_allocator();
        const auto marker = current.get_marker();
        [[maybe_unused]] auto* third = allocator.alloc<int>(99);
        current.free_to_marker(marker);

        REQUIRE(*first == 42);
        allocator.clear(0);
        allocator.swap_buffers();
        auto* fourth = allocator.alloc<int>(100);
        REQUIRE(*fourth == 100);
        allocator.free(fourth);
    }

    SECTION("TemplatedFree")
    {
        auto* obj = allocator.alloc<TestData>(42, 3.14f);
        REQUIRE(obj != nullptr);
        REQUIRE(obj->value == 42);

        // Should call destructor and free memory
        allocator.free(obj);

        // Should handle nullptr gracefully
        allocator.free<TestData>(nullptr);
    }
}

TEST_CASE("TripleBufferedAllocator", "[memory][stack_allocator]")
{
    portal::BufferedAllocator<3> allocator{1024};

    SECTION("ThreeBufferCycle")
    {
        auto* a = allocator.alloc<TestData>(1, 1.1f);
        REQUIRE(a->value == 1);

        allocator.swap_buffers(); // Buffer 1
        auto* b = allocator.alloc<TestData>(2, 2.2f);
        REQUIRE(b->value == 2);

        allocator.swap_buffers(); // Buffer 2
        auto* c = allocator.alloc<TestData>(3, 3.3f);
        REQUIRE(c->value == 3);

        allocator.swap_buffers(); // Back to buffer 0
        // Buffer 0 was cleared on swap, so a is invalid
        auto* d = allocator.alloc<TestData>(4, 4.4f);
        REQUIRE(d->value == 4);
        REQUIRE(b->value == 2); // Still valid in buffer 1
        REQUIRE(c->value == 3); // Still valid in buffer 2

        allocator.free(d);
    }

    SECTION("GetAllocators")
    {
        allocator.alloc<int>(1); // In buffer 0

        auto& current = allocator.get_current_allocator();
        auto& buffer0 = allocator.get_allocator(0);
        REQUIRE(&current == &buffer0);

        allocator.swap_buffers();
        auto& newCurrent = allocator.get_current_allocator();
        auto& buffer1 = allocator.get_allocator(1);
        REQUIRE(&newCurrent == &buffer1);
        REQUIRE(&newCurrent != &buffer0);
    }
}