cmake_minimum_required(VERSION 3.30)
project(portal-core)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "portal/core/*.cpp")
file(GLOB_RECURSE CORE_HEADER CONFIGURE_DEPENDS "portal/core/*.h")

find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_library(portal-core STATIC ${CORE_SOURCES})
add_library(portal::core ALIAS portal-core)

target_sources(portal-core PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${CORE_HEADER}
)

set_target_properties(portal-core PROPERTIES EXPORT_NAME core)

target_link_libraries(portal-core
        PUBLIC
        spdlog::spdlog
        glm::glm
)

install(TARGETS portal-core
        EXPORT portal-core-targets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        FILE_SET HEADERS DESTINATION include
)

# Generate and install the export targets
install(EXPORT portal-core-targets
        FILE portal-core-targets.cmake
        NAMESPACE portal::
        DESTINATION share/portal-core
)

# Create and install the config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/portal-core-config-version.cmake"
        VERSION 1.0.4
        COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
        "../cmake/portal-core-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/portal-core-config.cmake"
        INSTALL_DESTINATION share/portal-core
)

# Install the config files
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/portal-core-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/portal-core-config-version.cmake"
        DESTINATION share/portal-core
)