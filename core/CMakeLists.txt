cmake_minimum_required(VERSION 3.30)

file(READ "version.txt" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(portal-core VERSION ${PROJECT_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} - version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PORTAL_BUILD_TESTS "Whether or not to build the tests" ON)
option(PORTAL_FIND_PACAKGE "Whether or not to look for portal components" OFF) #OFF by default
option(PORTAL_PROFILE "Enable profiling with Tracy" ON)
option(PORTAL_BUILD_DIST "Build the distribution package" OFF)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "portal/core/*.cpp")
file(GLOB_RECURSE CORE_HEADER CONFIGURE_DEPENDS "portal/core/*.h")

file(GLOB_RECURSE LLVM_SOURCES CONFIGURE_DEPENDS "llvm/Support.cpp")
file(GLOB_RECURSE LLVM_HEADER CONFIGURE_DEPENDS "llvm/*.h" "llvm-c/*.h")

file(GLOB_RECURSE WINDOWS_SOURCES CONFIGURE_DEPENDS "portal/platform/core/windows/*.cpp")
file(GLOB_RECURSE WINDOWS_HEADER CONFIGURE_DEPENDS "portal/platform/core/windows/*.h")

file(GLOB_RECURSE MACOS_SOURCES CONFIGURE_DEPENDS "portal/platform/core/macos/*.cpp")
file(GLOB_RECURSE MACOS_HEADER CONFIGURE_DEPENDS "portal/platform/core/macos/*.h")

file(GLOB_RECURSE LINUX_SOURCES CONFIGURE_DEPENDS "portal/platform/core/linux/*.cpp")
file(GLOB_RECURSE LINUX_HEADER CONFIGURE_DEPENDS "portal/platform/core/linux/*.h")

find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Tracy CONFIG REQUIRED)

add_library(portal-core STATIC ${CORE_SOURCES} ${LLVM_SOURCES})
add_library(portal::core ALIAS portal-core)

target_sources(portal-core PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${CORE_HEADER} ${LLVM_HEADER}
)

set_target_properties(portal-core PROPERTIES EXPORT_NAME core)
target_link_libraries(portal-core
        PUBLIC
        spdlog::spdlog
        glm::glm
        Tracy::TracyClient
)

if (WIN32)
    set(PORTAL_PLATFORM_WINDOWS ON CACHE BOOL "Windows platform" FORCE)
    target_compile_definitions(portal-core PUBLIC PORTAL_PLATFORM_WINDOWS)
    target_sources(portal-core
            PRIVATE ${WINDOWS_SOURCES}
            PUBLIC
            FILE_SET HEADERS
            BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
            FILES ${WINDOWS_HEADER}
    )
    target_compile_definitions(portal-core PUBLIC PORTAL_PLATFORM=windows)

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
    target_compile_definitions(portal-core PUBLIC PORTAL_PLATFORM=macos)

elseif (UNIX)
    set(PORTAL_PLATFORM_LINUX ON CACHE BOOL "Linux platform" FORCE)
    target_compile_definitions(portal-core PUBLIC PORTAL_PLATFORM_LINUX)
    target_sources(portal-core
            PRIVATE ${LINUX_SOURCES}
            PUBLIC
            FILE_SET HEADERS
            BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
            FILES ${LINUX_HEADER}
    )
    target_compile_definitions(portal-core PUBLIC PORTAL_PLATFORM=unix)

else ()
    message(FATAL_ERROR "Unsupported platform")
endif ()

if (NOT PORTAL_BUILD_DIST)
    target_compile_definitions(portal-core PUBLIC
            $<$<CONFIG:Debug>:PORTAL_DEBUG>
            $<$<CONFIG:Release>:PORTAL_RELEASE>
    )
else ()
    target_compile_definitions(portal-core PUBLIC PORTAL_DIST)
endif ()

target_compile_definitions(portal-core PUBLIC
        GLM_FORCE_SWIZZLE
        GLM_FORCE_RADIANS
        GLM_FORCE_CTOR_INIT
        GLM_ENABLE_EXPERIMENTAL
        GLM_FORCE_CXX20
        SPDLOG_USE_STD_FORMAT
)

if (PORTAL_PROFILE AND NOT PORTAL_BUILD_DIST)
    target_compile_definitions(portal-core PUBLIC PORTAL_PROFILE $<$<CONFIG:Release>:TRACY_ENABLE>)
endif ()

if (MSVC)
    target_compile_options(portal-core PUBLIC
            "/Zc:__cplusplus"
            "$<$<CONFIG:Release>:/Zi>"
    )
    target_compile_options(portal-core PRIVATE /W4 /WX)

    target_link_options(portal-core PUBLIC
            "$<$<CONFIG:Release>:/DEBUG /OPT:REF /OPT:ICF>"
    )
else ()
    target_compile_options(portal-core PUBLIC
            "$<$<CONFIG:Release>:-g>"
    )
    target_compile_options(portal-core PRIVATE -Wall -Wextra -Wpedantic -Werror)

endif ()


if (PORTAL_BUILD_TESTS)
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