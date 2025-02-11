cmake_minimum_required(VERSION 3.30)
project(portal-networking)

set(CMAKE_CXX_STANDARD 20)
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

install(TARGETS portal-networking
        EXPORT portal-networking-targets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        FILE_SET HEADERS DESTINATION include
)

# Generate and install the export targets
install(EXPORT portal-networking-targets
        FILE portal-networking-targets.cmake
        NAMESPACE portal::
        DESTINATION share/portal-networking
)

# Create and install the config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/portal-networking-config-version.cmake"
        VERSION 1.0.0
        COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
        "../cmake/portal-networking-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/portal-networking-config.cmake"
        INSTALL_DESTINATION share/portal-networking
)

# Install the config files
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/portal-networking-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/portal-networking-config-version.cmake"
        DESTINATION share/portal-networking
)