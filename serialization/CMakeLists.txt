cmake_minimum_required(VERSION 3.30)
project(portal-serialization)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SER_SOURCES CONFIGURE_DEPENDS "portal/serialization/*.cpp")
file(GLOB_RECURSE SER_HEADERS CONFIGURE_DEPENDS "portal/serialization/*.h")

option(PORTAL_FIND_PACAKGE "Whether or not to look for portal components" OFF) #OFF by default
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
)

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
        VERSION 1.0.0
        COMPATIBILITY SameMajorVersion
)

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