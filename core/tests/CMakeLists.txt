file(GLOB_RECURSE TEST_SOURCES "*tests.cpp")

portal_add_test_target(portal-core
        SOURCES
        ${TEST_SOURCES}
        logger_listener.cpp
        config_impl.cpp
)