if(NOT Catch2_FOUND)
    find_package(Catch2 CONFIG REQUIRED)
endif()

enable_testing()

file(GLOB_RECURSE TEST_SOURCES "*tests.cpp")
file(GLOB_RECURSE TEST_HEADERS "*.h")

add_executable(portal-core-test ${TEST_SOURCES} ${TEST_HEADERS} logger_listener.cpp)
target_link_libraries(portal-core-test
        portal-core
        Catch2::Catch2
        Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(portal-core-test)