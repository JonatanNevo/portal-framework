if(NOT GTest_FOUND)
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    find_package(GTest REQUIRED)
endif()

find_package(benchmark CONFIG REQUIRED)

enable_testing()

file(GLOB_RECURSE TEST_SOURCES "*tests.cpp")
file(GLOB_RECURSE TEST_HEADERS "*.h")

file(GLOB_RECURSE BENCHMARK_SOURCES "*benchmark.cpp")

add_executable(portal-core-test ${TEST_SOURCES} ${TEST_HEADERS})
target_link_libraries(portal-core-test
        portal-core
        GTest::gtest
        GTest::gmock
)

include(GoogleTest)
gtest_discover_tests(portal-core-test)


add_executable(portal-core-benchmark ${BENCHMARK_SOURCES})
target_link_libraries(portal-core-benchmark
        portal-core
        benchmark::benchmark
)