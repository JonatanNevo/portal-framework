cmake_minimum_required(VERSION 3.30)

file(READ "version.txt" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(portal-networking VERSION ${PROJECT_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} - version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PORTAL_FIND_PACKAGE "Whether or not to look for portal components" OFF) #OFF by default

if (PORTAL_FIND_PACKAGE)
    find_package(portal-core CONFIG REQUIRED)
endif ()

file(GLOB_RECURSE NETWORKING_SOURCES CONFIGURE_DEPENDS "portal/networking/*.cpp")
file(GLOB_RECURSE NETWORKING_HEADERS CONFIGURE_DEPENDS "portal/networking/*.h")

portal_add_module(networking
        SOURCES ${NETWORKING_SOURCES}
        HEADERS ${NETWORKING_HEADERS}

        PORTAL_DEPENDENCIES
        core

        COMPLEX_DEPENDENCIES
        GameNetworkingSockets|LINK|GameNetworkingSockets::shared
)

portal_install_module(networking)