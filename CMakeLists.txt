cmake_minimum_required(VERSION 3.30)

project(portal_framework)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

add_subdirectory(core)
add_subdirectory(networking)
add_subdirectory(application)
add_subdirectory(input)
add_subdirectory(serialization)
add_subdirectory(engine)

add_subdirectory(examples/engine_test)

# API Documentation with Doxygen
option(PORTAL_BUILD_DOCS "Build API documentation with Doxygen" OFF)
if(PORTAL_BUILD_DOCS)
    add_subdirectory(docs)
endif()

enable_testing()
