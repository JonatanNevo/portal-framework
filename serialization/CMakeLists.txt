cmake_minimum_required(VERSION 3.30)

file(READ "version" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(portal-serialization VERSION ${PROJECT_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} - version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SER_SOURCES CONFIGURE_DEPENDS "portal/serialization/*.cpp")
file(GLOB_RECURSE SER_HEADERS CONFIGURE_DEPENDS "portal/serialization/*.h")
file(GLOB_RECURSE SER_IMP_SOURCES CONFIGURE_DEPENDS "portal/serialization/impl/*.cpp")
file(GLOB_RECURSE SER_IMP_HEADERS CONFIGURE_DEPENDS "portal/serialization/impl/*.h")

find_package(nlohmann_json CONFIG REQUIRED)

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
        nlohmann_json::nlohmann_json
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