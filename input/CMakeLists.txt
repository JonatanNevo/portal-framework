cmake_minimum_required(VERSION 3.30)

file(READ "version.txt" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(portal-input VERSION ${PROJECT_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} - version: ${PROJECT_VERSION}")

include(../cmake/development.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE INPUT_SOURCES CONFIGURE_DEPENDS "portal/input/*.cpp")
file(GLOB_RECURSE INPUT_HEADERS CONFIGURE_DEPENDS "portal/input/*.h")

file(GLOB_RECURSE GENERIC_SOURCES CONFIGURE_DEPENDS "portal/platform/input/generic/*.cpp")
file(GLOB_RECURSE GENERIC_HEADERS CONFIGURE_DEPENDS "portal/platform/input/generic/*.h")
file(GLOB_RECURSE HAL_INCLUDES CONFIGURE_DEPENDS "portal/platform/input/generic/hal/*.h")

file(GLOB_RECURSE WINDOWS_SOURCES CONFIGURE_DEPENDS "portal/platform/input/windows/*.cpp")
file(GLOB_RECURSE WINDOWS_HEADER CONFIGURE_DEPENDS "portal/platform/input/windows/*.h")

file(GLOB_RECURSE MACOS_SOURCES CONFIGURE_DEPENDS "portal/platform/input/macos/*.cpp")
file(GLOB_RECURSE MACOS_HEADER CONFIGURE_DEPENDS "portal/platform/input/macos/*.h")

file(GLOB_RECURSE LINUX_SOURCES CONFIGURE_DEPENDS "portal/platform/input/linux/*.cpp")
file(GLOB_RECURSE LINUX_HEADER CONFIGURE_DEPENDS "portal/platform/input/linux/*.h")

find_package(glfw3 CONFIG REQUIRED)
find_package(glaze CONFIG REQUIRED)

option(PORTAL_FIND_PACAKGE "Whether or not to look for portal components" OFF) #OFF by default
if(PORTAL_FIND_PACAKGE)
    find_package(portal-core CONFIG REQUIRED)
endif ()
unset(PORTAL_FIND_PACAKGE CACHE)

add_library(portal-input STATIC ${INPUT_SOURCES} ${GENERIC_SOURCES})
add_library(portal::input ALIAS portal-input)

target_sources(portal-input PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${INPUT_HEADERS} ${GENERIC_HEADERS} ${HAL_INCLUDES}
)

if (WIN32)
    set(PORTAL_PLATFORM_WINDOWS ON CACHE BOOL "Windows platform" FORCE)
    target_compile_definitions(portal-input PUBLIC PORTAL_PLATFORM_WINDOWS)
    target_sources(portal-input
            PRIVATE ${WINDOWS_SOURCES}
            PUBLIC
            FILE_SET HEADERS
            BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
            FILES ${WINDOWS_HEADER}
    )
elseif (APPLE)
    set(PORTAL_PLATFORM_MACOS ON CACHE BOOL "MacOS platform" FORCE)
    target_compile_definitions(portal-input PUBLIC PORTAL_PLATFORM_MACOS)
    target_sources(portal-input
            PRIVATE ${MACOS_SOURCES}
            PUBLIC
            FILE_SET HEADERS
            BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
            FILES ${MACOS_HEADER}
    )
elseif (UNIX)
    set(PORTAL_PLATFORM_LINUX ON CACHE BOOL "Linux platform" FORCE)
    target_compile_definitions(portal-input PUBLIC PORTAL_PLATFORM_LINUX)
    target_sources(portal-input
            PRIVATE ${LINUX_SOURCES}
            PUBLIC
            FILE_SET HEADERS
            BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
            FILES ${LINUX_HEADER}
    )
endif ()

set_target_properties(portal-input PROPERTIES EXPORT_NAME input)

target_link_libraries(portal-input
        PUBLIC
        portal::core
        glaze::glaze
        glfw
)

if(MSVC)
    target_compile_options(portal-input PRIVATE /W4 /WX)
else()
    target_compile_options(portal-input PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-missing-designated-field-initializers)
endif()

prepare_consteval_strings(portal-input ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_BINARY_DIR}/input_map.inc)


install(TARGETS portal-input
        EXPORT portal-input-targets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        FILE_SET HEADERS DESTINATION include
)

# Generate and install the export targets
install(EXPORT portal-input-targets
        FILE portal-input-targets.cmake
        NAMESPACE portal::
        DESTINATION share/portal-input
)

# Create and install the config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/portal-input-config-version.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)
unset(PROJECT_VERSION)

configure_package_config_file(
        "../cmake/portal-input-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/portal-input-config.cmake"
        INSTALL_DESTINATION share/portal-input
)

# Install the config files
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/portal-input-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/portal-input-config-version.cmake"
        DESTINATION share/portal-input
)