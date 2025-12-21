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

find_package(nlohmann_json CONFIG REQUIRED)

file(GLOB_RECURSE SER_SOURCES CONFIGURE_DEPENDS "portal/serialization/*.cpp")
file(GLOB_RECURSE SER_HEADERS CONFIGURE_DEPENDS "portal/serialization/*.h")

if(PORTAL_FIND_PACKAGE)
    find_package(portal-core CONFIG REQUIRED)
endif ()
unset(PORTAL_FIND_PACKAGE CACHE)

add_library(portal-serialization STATIC ${SER_SOURCES})
target_sources(portal-serialization PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${SER_HEADERS}
)
set_target_properties(portal-serialization PROPERTIES EXPORT_NAME serialization)

add_library(portal::serialization ALIAS portal-serialization)

target_link_libraries(portal-serialization
        PUBLIC
        portal::core
        nlohmann_json::nlohmann_json
)

if(MSVC)
    target_compile_options(portal-serialization PRIVATE /W4 /WX)
else()
    target_compile_options(portal-serialization PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-missing-designated-field-initializers)
endif()

enable_testing()
portal_build_tests(tests)

portal_install_target(portal-serialization)