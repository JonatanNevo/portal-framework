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


add_executable(portal-engine-test ${TEST_SOURCES} ${TEST_HEADERS} test_main.cpp)
target_link_libraries(portal-engine-test
        portal-engine
        GTest::gtest
        GTest::gmock
)

include(GoogleTest)
gtest_discover_tests(portal-engine-test)


add_executable(portal-engine-benchmark ${BENCHMARK_SOURCES} bench_main.cpp)
target_link_libraries(portal-engine-benchmark
        portal-engine
        benchmark::benchmark
)