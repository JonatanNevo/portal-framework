cmake_minimum_required(VERSION 3.30)

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

if(PORTAL_BUILD_DOCS)
    add_subdirectory(docs)
endif()

include(CPack)