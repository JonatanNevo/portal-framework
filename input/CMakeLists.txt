cmake_minimum_required(VERSION 3.30)

file(READ "version.txt" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(portal-input VERSION ${PROJECT_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} - version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PORTAL_FIND_PACKAGE "Whether or not to look for portal components" OFF) #OFF by default

if (PORTAL_FIND_PACKAGE)
    find_package(portal-core CONFIG REQUIRED)
endif ()

file(GLOB_RECURSE INPUT_SOURCES CONFIGURE_DEPENDS "portal/input/*.cpp")
file(GLOB_RECURSE INPUT_HEADERS CONFIGURE_DEPENDS "portal/input/*.h")

portal_add_module(input
        SOURCES ${INPUT_SOURCES}
        HEADERS ${INPUT_HEADERS}

        PORTAL_DEPENDENCIES
            core
            application

        COMPLEX_DEPENDENCIES
            glfw3|LINK|glfw
)

portal_install_module(input)