cmake_minimum_required(VERSION 3.30)
project(portal_framework)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    add_compile_definitions(PORTAL_PLATFORM_WINDOWS)
elseif(APPLE)
    add_compile_definitions(PORTAL_PLATFORM_MACOS)
else()
    add_compile_definitions(PORTAL_PLATFORM_LINUX)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(PORTAL_DEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(PORTAL_RELEASE)
else()
    add_compile_definitions(PORTAL_DIST)
endif()

add_subdirectory(core)
add_subdirectory(networking)

add_executable(portal main.cpp)
target_link_libraries(portal PUBLIC portal::networking)