cmake_minimum_required(VERSION 3.30)

# Experimental packaging tools
set(CMAKE_EXPERIMENTAL_EXPORT_PACKAGE_DEPENDENCIES 1942b4fa-b2c5-4546-9385-83f254070067)

file(READ "version.txt" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(portal-application VERSION ${PROJECT_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} - version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "portal/application/*.cpp")
file(GLOB_RECURSE APP_HEADERS CONFIGURE_DEPENDS "portal/application/*.h")

option(PORTAL_FIND_PACKAGE "Whether or not to look for portal components" OFF) #OFF by default
if(PORTAL_FIND_PACKAGE)
    find_package(portal-core CONFIG REQUIRED)
endif ()
unset(PORTAL_FIND_PACKAGE CACHE)

add_library(portal-application STATIC ${APP_SOURCES})
target_sources(portal-application PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${APP_HEADERS}
)

set_target_properties(portal-application PROPERTIES EXPORT_NAME application)

add_library(portal::application ALIAS portal-application)

target_link_libraries(portal-application
        PUBLIC
        portal::core
)

target_include_directories(portal-application PRIVATE ${Stb_INCLUDE_DIR})

if(MSVC)
    target_compile_options(portal-application PRIVATE /W4 /WX)
else()
    target_compile_options(portal-application PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-missing-designated-field-initializers)
endif()

install(
        TARGETS portal-application
        EXPORT portal-application-targets
        FILE_SET HEADERS
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/portal-application-version.cmake"
        COMPATIBILITY ExactVersion
)

install(
        EXPORT portal-application-targets
        FILE portal-application-config.cmake
        NAMESPACE portal::
        DESTINATION share/portal-application
        EXPORT_PACKAGE_DEPENDENCIES
)

install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/portal-application-version.cmake"
        DESTINATION share/portal-application
)