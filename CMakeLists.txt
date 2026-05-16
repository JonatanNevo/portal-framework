cmake_minimum_required(VERSION 3.30)

# Load .env before project() so the vcpkg toolchain picks up
# VCPKG_BINARY_SOURCES / VCPKG_NUGET_REPOSITORY when it loads.
if(EXISTS "${CMAKE_SOURCE_DIR}/.env")
    file(STRINGS "${CMAKE_SOURCE_DIR}/.env" _portal_env_lines)
    foreach(_portal_env_line IN LISTS _portal_env_lines)
        if(_portal_env_line MATCHES "^[ \t]*#" OR _portal_env_line STREQUAL "")
            continue()
        endif()
        if(_portal_env_line MATCHES "^[ \t]*([A-Za-z_][A-Za-z0-9_]*)[ \t]*=(.*)$")
            set(_portal_env_key "${CMAKE_MATCH_1}")
            set(_portal_env_val "${CMAKE_MATCH_2}")
            string(REGEX REPLACE "^\"(.*)\"$" "\\1" _portal_env_val "${_portal_env_val}")
            string(REGEX REPLACE "^'(.*)'$" "\\1" _portal_env_val "${_portal_env_val}")
            if(NOT DEFINED ENV{${_portal_env_key}})
                set(ENV{${_portal_env_key}} "${_portal_env_val}")
            endif()
        endif()
    endforeach()
    unset(_portal_env_lines)
    unset(_portal_env_line)
    unset(_portal_env_key)
    unset(_portal_env_val)
endif()

project(portal_framework)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(PORTAL_BUILD_DOCS "Build API documentation with Doxygen" OFF)
if(PORTAL_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    if(NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is required to build API documentation. Please install Doxygen.")
    endif()

    set_property(GLOBAL PROPERTY PORTAL_DOCS_TARGETS "")
    set(PORTAL_DOCS_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/docs/build)
endif()

enable_testing()
add_subdirectory(core)
add_subdirectory(serialization)
add_subdirectory(application)
add_subdirectory(input)
add_subdirectory(networking)
add_subdirectory(engine)

add_subdirectory(examples/engine_test)
add_subdirectory(examples/shader_playground)

if(PORTAL_BUILD_DOCS)
    add_subdirectory(docs)
endif()

include(CPack)