if(NOT Catch2_FOUND)
    find_package(Catch2 CONFIG REQUIRED)
endif()

enable_testing()

file(GLOB_RECURSE TEST_SOURCES "*tests.cpp")
file(GLOB_RECURSE TEST_HEADERS "*.h")


add_executable(portal-engine-test ${TEST_SOURCES} ${TEST_HEADERS})
target_link_libraries(portal-engine-test
        portal-engine
        Catch2::Catch2
        Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(portal-engine-test)