cmake_minimum_required(VERSION 3.30)

file(READ "version.txt" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(portal-application VERSION ${PROJECT_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} - version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PORTAL_FIND_PACKAGE "Whether or not to look for portal components" OFF) #OFF by default
if (PORTAL_FIND_PACKAGE)
    find_package(portal-core CONFIG REQUIRED)
    find_package(portal-serialization)
endif ()

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "portal/application/*.cpp")
file(GLOB_RECURSE APP_HEADERS CONFIGURE_DEPENDS "portal/application/*.h")

portal_add_module(application
        SOURCES ${APP_SOURCES}
        HEADERS ${APP_HEADERS}

        PORTAL_DEPENDENCIES
            core
            serialization
)

target_include_directories(portal-application PRIVATE ${Stb_INCLUDE_DIR})

portal_install_module(application)