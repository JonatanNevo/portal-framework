file(GLOB_RECURSE TEST_SOURCES "*tests.cpp")

portal_add_test_target(portal-serialization
        SOURCES
        ${TEST_SOURCES}
        logger_listener.cpp
)

add_executable(something something.cpp)
target_link_libraries(something PRIVATE portal-serialization)