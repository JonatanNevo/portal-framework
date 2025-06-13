cmake_minimum_required(VERSION 3.30)

file(READ "version.txt" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(portal-engine VERSION ${PROJECT_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} - version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS "portal/engine/*.cpp")
file(GLOB_RECURSE ENGINE_HEADER CONFIGURE_DEPENDS "portal/engine/*.h")

option(PORTAL_BUILD_TESTS "Whether or not to build the tests" ON)

option(PORTAL_FIND_PACAKGE "Whether or not to look for portal components" OFF) #OFF by default
if(PORTAL_FIND_PACAKGE)
    find_package(portal-application CONFIG REQUIRED)
    find_package(portal-input CONFIG REQUIRED)
    find_package(portal-networking CONFIG REQUIRED)
    find_package(portal-serialization CONFIG REQUIRED)
endif ()
unset(PORTAL_FIND_PACAKGE CACHE)


add_library(portal-engine STATIC ${ENGINE_SOURCES})
target_sources(portal-engine PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${ENGINE_HEADERS}
)

set_target_properties(portal-engine PROPERTIES EXPORT_NAME engine)

add_library(portal::engine ALIAS portal-engine)

target_link_libraries(portal-engine
        PUBLIC
        portal::application
        portal::input
        portal::networking
        portal::serialization
)

if(MSVC)
    target_compile_options(portal-networking PRIVATE /W4 /WX)
else()
    target_compile_options(portal-networking PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(PORTAL_BUILD_TESTS)
    add_subdirectory(tests)
endif ()
unset(PORTAL_BUILD_TESTS CACHE)

install(TARGETS portal-engine
        EXPORT portal-engine-targets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        FILE_SET HEADERS DESTINATION include
)

# Generate and install the export targets
install(EXPORT portal-engine-targets
        FILE portal-engine-targets.cmake
        NAMESPACE portal::
        DESTINATION share/portal-engine
)

# Create and install the config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/portal-engine-config-version.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)
# To free it up for other add_subdirectories
unset(PROJECT_VERSION)

configure_package_config_file(
        "../cmake/portal-engine-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/portal-engine-config.cmake"
        INSTALL_DESTINATION share/portal-engine
)

# Install the config files
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/portal-engine-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/portal-engine-config-version.cmake"
        DESTINATION share/portal-engine
)

add_executable(engine-test main.cpp)
target_link_libraries(engine-test
        PUBLIC
        portal::engine
)