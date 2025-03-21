cmake_minimum_required(VERSION 3.30)

file(READ "version" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(portal-core VERSION ${PROJECT_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} - version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB CORE_SOURCES CONFIGURE_DEPENDS "portal/core/*.cpp")
file(GLOB CORE_HEADER CONFIGURE_DEPENDS "portal/core/*.h")

file(GLOB_RECURSE WINDOWS_SOURCES CONFIGURE_DEPENDS "portal/platform/windows/*.cpp")
file(GLOB_RECURSE WINDOWS_HEADER CONFIGURE_DEPENDS "portal/platform/windows/*.h")

file(GLOB_RECURSE MACOS_SOURCES CONFIGURE_DEPENDS "portal/platform/macos/*.cpp")
file(GLOB_RECURSE MACOS_HEADER CONFIGURE_DEPENDS "portal/platform/macos/*.h")

file(GLOB_RECURSE LINUX_SOURCES CONFIGURE_DEPENDS "portal/platform/linux/*.cpp")
file(GLOB_RECURSE LINUX_HEADER CONFIGURE_DEPENDS "portal/platform/linux/*.h")

option(PORTAL_BUILD_TESTS "Whether or not to build the tests" OFF)

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

if(WIN32)
    set(PORTAL_PLATFORM_WINDOWS ON CACHE BOOL "Windows platform" FORCE)
    target_compile_definitions(portal-core PUBLIC PORTAL_PLATFORM_WINDOWS)
    target_sources(portal-core
            PRIVATE ${WINDOWS_SOURCES}
            PUBLIC
            FILE_SET HEADERS
            BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
            FILES ${WINDOWS_HEADER}
    )

elseif (APPLE)
    set(PORTAL_PLATFORM_MACOS ON CACHE BOOL "MacOS platform" FORCE)
    target_compile_definitions(portal-core PUBLIC PORTAL_PLATFORM_MACOS)
    target_sources(portal-core
            PRIVATE ${MACOS_SOURCES}
            PUBLIC
            FILE_SET HEADERS
            BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
            FILES ${MACOS_HEADER}
    )

elseif(UNIX)
    set(PORTAL_PLATFORM_LINUX ON CACHE BOOL "Linux platform" FORCE)
    target_compile_definitions(portal-core PUBLIC PORTAL_PLATFORM_LINUX)
    target_sources(portal-core
            PRIVATE ${LINUX_SOURCES}
            PUBLIC
            FILE_SET HEADERS
            BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
            FILES ${LINUX_HEADER}
    )

else()
    message(FATAL_ERROR "Unsupported platform")
endif()

target_compile_definitions(portal-core PUBLIC
        $<$<CONFIG:Debug>:PORTAL_DEBUG>
        $<$<CONFIG:Release>:PORTAL_RELEASE>
        $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:PORTAL_DIST>
)

target_compile_definitions(portal-core PUBLIC
        GLM_FORCE_SWIZZLE
        GLM_FORCE_RADIANS
        GLM_FORCE_CTOR_INIT
        GLM_ENABLE_EXPERIMENTAL
        GLM_FORCE_CXX20
)

if(MSVC)
    target_compile_options(portal-core PUBLIC
            "/Zc:__cplusplus"
            "$<$<CONFIG:Release>:/Zi>"
    )

    target_link_options(portal-core PUBLIC
            "$<$<CONFIG:Release>:/DEBUG /OPT:REF /OPT:ICF>"
    )
else()
    target_compile_options(portal-core PUBLIC
            "$<$<CONFIG:Release>:-g>"
    )
endif()


if(PORTAL_BUILD_TESTS)
    add_subdirectory(tests)
endif ()
unset(PORTAL_BUILD_TESTS CACHE)

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
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)
unset(PROJECT_VERSION)

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