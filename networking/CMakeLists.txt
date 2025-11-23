cmake_minimum_required(VERSION 3.30)

file(READ "version.txt" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(portal-networking VERSION ${PROJECT_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} - version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE NETWORKING_SOURCES CONFIGURE_DEPENDS "portal/networking/*.cpp")
file(GLOB_RECURSE NETWORKING_HEADERS CONFIGURE_DEPENDS "portal/networking/*.h")

find_package(GameNetworkingSockets CONFIG REQUIRED)

option(PORTAL_FIND_PACAKGE "Whether or not to look for portal components" OFF) #OFF by default
if(PORTAL_FIND_PACAKGE)
    find_package(portal-core CONFIG REQUIRED)
endif ()
unset(PORTAL_FIND_PACAKGE CACHE)

add_library(portal-networking STATIC ${NETWORKING_SOURCES})
add_library(portal::networking ALIAS portal-networking)

target_sources(portal-networking PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${NETWORKING_HEADERS}
)

set_target_properties(portal-networking PROPERTIES EXPORT_NAME networking)

target_link_libraries(portal-networking
        PUBLIC
        portal::core
        GameNetworkingSockets::shared
)

if(MSVC)
    target_compile_options(portal-networking PRIVATE /W4 /WX)
else()
    target_compile_options(portal-networking PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-missing-designated-field-initializers)
endif()

install(
        TARGETS portal-networking
        EXPORT portal-networking-targets
        FILE_SET HEADERS
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/portal-networking-version.cmake"
        COMPATIBILITY ExactVersion
)

install(
        EXPORT portal-networking-targets
        FILE portal-networking-config.cmake
        NAMESPACE portal::
        DESTINATION share/portal-networking
        EXPORT_PACKAGE_DEPENDENCIES
)

install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/portal-networking-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/portal-networking-version.cmake"
        DESTINATION share/portal-networking
)