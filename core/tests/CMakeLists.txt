if(NOT GTest_FOUND)
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    find_package(GTest REQUIRED)
endif()

enable_testing()

file(GLOB_RECURSE TEST_SOURCES "*tests.cpp")
file(GLOB_RECURSE TEST_HEADERS "*.h")

add_executable(portal-core-test ${TEST_SOURCES} ${TEST_HEADERS} main.cpp)
target_link_libraries(portal-core-test
        portal-core
        GTest::gtest
        GTest::gmock
)

include(GoogleTest)
gtest_discover_tests(portal-core-test)