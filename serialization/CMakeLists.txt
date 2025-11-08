cmake_minimum_required(VERSION 3.30)

file(READ "version.txt" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(portal-serialization VERSION ${PROJECT_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} - version: ${PROJECT_VERSION}")

include(../cmake/development.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PORTAL_BUILD_TESTS "Whether or not to build the tests" ON)
option(PORTAL_FIND_PACAKGE "Whether or not to look for portal components" OFF) #OFF by default

find_package(nlohmann_json CONFIG REQUIRED)

file(GLOB_RECURSE SER_SOURCES CONFIGURE_DEPENDS "portal/serialization/*.cpp")
file(GLOB_RECURSE SER_HEADERS CONFIGURE_DEPENDS "portal/serialization/*.h")

if(PORTAL_FIND_PACAKGE)
    find_package(portal-core CONFIG REQUIRED)
endif ()
unset(PORTAL_FIND_PACAKGE CACHE)

add_library(portal-serialization STATIC ${SER_SOURCES})
target_sources(portal-serialization PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${SER_HEADERS}
)
set_target_properties(portal-serialization PROPERTIES EXPORT_NAME serialization)

add_library(portal::serialization ALIAS portal-serialization)

target_link_libraries(portal-serialization
        PUBLIC
        portal::core
        nlohmann_json::nlohmann_json
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    target_compile_options(portal-serialization PRIVATE /W4 /WX)
else()
    target_compile_options(portal-serialization PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-missing-designated-field-initializers)
endif()

if (PORTAL_BUILD_TESTS)
    add_subdirectory(tests)
endif ()
unset(PORTAL_BUILD_TESTS CACHE)

install(TARGETS portal-serialization
        EXPORT portal-serialization-targets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        FILE_SET HEADERS DESTINATION include
)

# Generate and install the export targets
install(EXPORT portal-serialization-targets
        FILE portal-serialization-targets.cmake
        NAMESPACE portal::
        DESTINATION share/portal-serialization
)

# Create and install the config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/portal-serialization-config-version.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)
unset(PROJECT_VERSION)

configure_package_config_file(
        "../cmake/portal-serialization-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/portal-serialization-config.cmake"
        INSTALL_DESTINATION share/portal-serialization
)

# Install the config files
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/portal-serialization-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/portal-serialization-config-version.cmake"
        DESTINATION share/portal-serialization
)