cmake_minimum_required(VERSION 3.30)

file(READ "version.txt" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(portal-serialization VERSION ${PROJECT_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} - version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PORTAL_BUILD_TESTS "Whether or not to build the tests" ON)
option(PORTAL_FIND_PACKAGE "Whether or not to look for portal components" OFF) #OFF by default

if (PORTAL_FIND_PACKAGE)
    find_package(portal-core CONFIG REQUIRED)
endif ()

file(GLOB_RECURSE SER_SOURCES CONFIGURE_DEPENDS "portal/serialization/*.cpp")
file(GLOB_RECURSE SER_HEADERS CONFIGURE_DEPENDS "portal/serialization/*.h")

portal_add_module(serialization
        SOURCES ${SER_SOURCES}
        HEADERS ${SER_HEADERS}

        PORTAL_DEPENDENCIES
        core

        PORTAL_FIND_PACKAGE ${PORTAL_FIND_PACKAGE}

        DEPENDENCIES
        nlohmann_json
        glaze
)

enable_testing()
portal_build_tests(tests)

portal_install_module(serialization)