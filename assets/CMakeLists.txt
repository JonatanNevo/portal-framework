cmake_minimum_required(VERSION 3.30)
project(portal-assets)

include(../cmake/development.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE ASSETS_SOURCES CONFIGURE_DEPENDS "portal/assets/*.cpp")
file(GLOB_RECURSE ASSETS_HEADERS CONFIGURE_DEPENDS "portal/assets/*.h")

option(PORTAL_FIND_PACAKGE "Whether or not to look for portal components" OFF) #OFF by default

find_package(unofficial-concurrentqueue CONFIG REQUIRED)

if(PORTAL_FIND_PACAKGE)
    find_package(portal-core CONFIG REQUIRED)
    find_package(portal-serialization CONFIG REQUIRED)
endif ()
unset(PORTAL_FIND_PACAKGE CACHE)

add_library(portal-assets STATIC ${ASSETS_SOURCES}
        portal/assets/asset_extensions.h)
target_sources(portal-assets PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${ASSETS_HEADERS}
)

set_target_properties(portal-assets PROPERTIES EXPORT_NAME assets)

add_library(portal::assets ALIAS portal-assets)

target_link_libraries(portal-assets
        PUBLIC
        portal::core
        portal::serialization
        unofficial::concurrentqueue::concurrentqueue
)

install(TARGETS portal-assets
        EXPORT portal-assets-targets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        FILE_SET HEADERS DESTINATION include
)

if(MSVC)
    target_compile_options(portal-assets PRIVATE /W4 /WX)
else()
    target_compile_options(portal-assets PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-missing-designated-field-initializers)
endif()

# Generate and install the export targets
install(EXPORT portal-assets-targets
        FILE portal-assets-targets.cmake
        NAMESPACE portal::
        DESTINATION share/portal-assets
)

# Create and install the config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/portal-assets-config-version.cmake"
        VERSION 1.0.0
        COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
        "../cmake/portal-assets-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/portal-assets-config.cmake"
        INSTALL_DESTINATION share/portal-assets
)

# Install the config files
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/portal-assets-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/portal-assets-config-version.cmake"
        DESTINATION share/portal-assets
)