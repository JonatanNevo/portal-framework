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
option(PORTAL_SANITIZE "Add sanitizers to build" OFF)
option(PORTAL_ASAN "Add address sanitizers (mutually exclusive to PORTAL_TSAN)" OFF)
option(PORTAL_TSAN "Add thread sanitizers (mutually exclusive to PORTAL_ASAN)" OFF)

if(PORTAL_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    if(NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is required to build API documentation. Please install Doxygen.")
    endif()

    set_property(GLOBAL PROPERTY PORTAL_DOCS_TARGETS "")
    set(PORTAL_DOCS_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/docs/build)
endif()

if(PORTAL_SANITIZE)
    if(PORTAL_ASAN AND PORTAL_TSAN)
        message(FATAL_ERROR "Could not enable both ASAN and TSAN")
    endif ()

    if (PORTAL_ASAN)
        message(STATUS "Enabling address sanitizers")

        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    elseif (PORTAL_TSAN)
        message(STATUS "Enabling thread sanitizers")

        add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
        add_link_options(-fsanitize=thread)

        # moodycamel's concurrentqueue uses std::atomic_thread_fence, which gcc
        # warns can't be fully modelled by TSan. Demote the warning so the build
        # succeeds; TSan still catches races outside the queue fences.
        add_compile_options(-Wno-error=tsan)
    endif ()
endif()

enable_testing()
add_subdirectory(core)
add_subdirectory(engine-core)
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