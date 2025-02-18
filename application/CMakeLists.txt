cmake_minimum_required(VERSION 3.30)
project(portal-application)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "portal/application/*.cpp")
file(GLOB_RECURSE APP_HEADERS CONFIGURE_DEPENDS "portal/application/*.h")

option(PORTAL_FIND_PACAKGE "Whether or not to look for portal components" OFF) #OFF by default
if(PORTAL_FIND_PACAKGE)
    find_package(portal-core CONFIG REQUIRED)
endif ()
unset(PORTAL_FIND_PACAKGE CACHE)

add_library(portal-application STATIC ${APP_SOURCES} ${HEADLESS_SOURCES})
target_sources(portal-application PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${HEADLESS_HEADERS}
)

set_target_properties(portal-application PROPERTIES EXPORT_NAME application)

add_library(portal::application ALIAS portal-application)

target_sources(portal-application PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${APP_HEADERS}
)

target_link_libraries(portal-application
        PUBLIC
        portal::core
)

target_include_directories(portal-application PRIVATE ${Stb_INCLUDE_DIR})

install(TARGETS portal-application
        EXPORT portal-application-targets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        FILE_SET HEADERS DESTINATION include
)

# Generate and install the export targets
install(EXPORT portal-application-targets
        FILE portal-application-targets.cmake
        NAMESPACE portal::
        DESTINATION share/portal-application
)

# Create and install the config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/portal-application-config-version.cmake"
        VERSION 1.0.0
        COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
        "../cmake/portal-application-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/portal-application-config.cmake"
        INSTALL_DESTINATION share/portal-application
)

# Install the config files
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/portal-application-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/portal-application-config-version.cmake"
        DESTINATION share/portal-application
)