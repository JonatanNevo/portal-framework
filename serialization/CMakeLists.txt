cmake_minimum_required(VERSION 3.30)

# Experimental packaging tools
set(CMAKE_EXPERIMENTAL_EXPORT_PACKAGE_DEPENDENCIES 1942b4fa-b2c5-4546-9385-83f254070067)

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

        DEPENDENCIES
        nlohmann_json
)

enable_testing()
portal_build_tests(tests)

portal_install_module(serialization)