cmake_minimum_required(VERSION 3.30)

file(READ "version.txt" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(portal-renderer VERSION ${PROJECT_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} - version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE RENDERER_SOURCES CONFIGURE_DEPENDS "portal/renderer/*.cpp")
file(GLOB_RECURSE RENDERER_HEADER CONFIGURE_DEPENDS "portal/renderer/*.h")

find_package(imgui CONFIG REQUIRED)
find_package(Stb REQUIRED)

option(PORTAL_FIND_PACAKGE "Whether or not to look for portal components" OFF) #OFF by default
if(PORTAL_FIND_PACAKGE)
    find_package(portal-application CONFIG REQUIRED)
    find_package(portal-assets CONFIG REQUIRED)
endif ()
unset(PORTAL_FIND_PACAKGE CACHE)

add_library(portal-renderer STATIC ${RENDERER_SOURCES})
target_sources(portal-renderer PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${RENDERER_HEADERS}
)

set_target_properties(portal-renderer PROPERTIES EXPORT_NAME renderer)

add_library(portal::renderer ALIAS portal-renderer)

target_link_libraries(portal-renderer
        PUBLIC
        portal::application
        portal::assets
        imgui::imgui
)

target_include_directories(portal-renderer PRIVATE ${Stb_INCLUDE_DIR})


if(MSVC)
    target_compile_options(portal-networking PRIVATE /W4 /WX)
else()
    target_compile_options(portal-networking PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

install(TARGETS portal-renderer
        EXPORT portal-renderer-targets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        FILE_SET HEADERS DESTINATION include
)

# Generate and install the export targets
install(EXPORT portal-renderer-targets
        FILE portal-renderer-targets.cmake
        NAMESPACE portal::
        DESTINATION share/portal-renderer
)

# Create and install the config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/portal-renderer-config-version.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)
# To free it up for other add_subdirectories
unset(PROJECT_VERSION)

configure_package_config_file(
        "../cmake/portal-renderer-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/portal-renderer-config.cmake"
        INSTALL_DESTINATION share/portal-renderer
)

# Install the config files
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/portal-renderer-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/portal-renderer-config-version.cmake"
        DESTINATION share/portal-renderer
)