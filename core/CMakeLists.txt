cmake_minimum_required(VERSION 3.30)

file(READ "version.txt" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(portal-core VERSION ${PROJECT_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} - version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PORTAL_BUILD_TESTS "Whether or not to build the tests" ON)
option(PORTAL_DEBUG_ALLOCATIONS "Enable debug allocations (for debug only)" OFF)
option(PORTAL_PROFILE "Enable profiling with Tracy" OFF)

include(cmake/portal-test-helpers.cmake)
include(cmake/portal-install-helpers.cmake)
include(cmake/portal-module-helpers.cmake)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "portal/core/*.cpp")
file(GLOB_RECURSE CORE_HEADERS CONFIGURE_DEPENDS "portal/core/*.h")

file(GLOB_RECURSE CORE_HAL_HEADERS CONFIGURE_DEPENDS "portal/platform/core/hal/*.h")

file(GLOB_RECURSE CORE_WINDOWS_SOURCES CONFIGURE_DEPENDS "portal/platform/core/windows/*.cpp")
file(GLOB_RECURSE CORE_WINDOWS_HEADERS CONFIGURE_DEPENDS "portal/platform/core/windows/*.h")

file(GLOB_RECURSE CORE_MACOS_SOURCES CONFIGURE_DEPENDS "portal/platform/core/macos/*.cpp")
file(GLOB_RECURSE CORE_MACOS_HEADERS CONFIGURE_DEPENDS "portal/platform/core/macos/*.h")

file(GLOB_RECURSE CORE_LINUX_SOURCES CONFIGURE_DEPENDS "portal/platform/core/linux/*.cpp")
file(GLOB_RECURSE CORE_LINUX_HEADERS CONFIGURE_DEPENDS "portal/platform/core/linux/*.h")

portal_add_module(core
        SOURCES ${CORE_SOURCES}
        HEADERS ${CORE_HEADERS} ${CORE_HAL_HEADERS}

        WINDOWS_SOURCES ${CORE_WINDOWS_SOURCES}
        WINDOWS_HEADERS ${CORE_WINDOWS_HEADERS}

        MACOS_SOURCES ${CORE_MACOS_SOURCES}
        MACOS_HEADERS ${CORE_MACOS_HEADERS}

        LINUX_SOURCES ${CORE_LINUX_SOURCES}
        LINUX_HEADERS ${CORE_LINUX_HEADERS}

        DEPENDENCIES
            glm
            spdlog
            enchantum
            frozen
            concurrentqueue
            nfd

        COMPLEX_DEPENDENCIES
            llvm-adt|LINK|llvm::adt

        COMPILE_CONFIG_FILE
            portal/core/config.h
)

target_compile_definitions(portal-core PUBLIC
        GLM_FORCE_SWIZZLE
        GLM_FORCE_RADIANS
        GLM_FORCE_CTOR_INIT
        GLM_ENABLE_EXPERIMENTAL
        GLM_FORCE_CXX20
        GLM_FORCE_DEPTH_ZERO_TO_ONE
)

if (WIN32)
    set(PORTAL_PLATFORM_WINDOWS ON)
    set(PORTAL_PLATFORM WINDOWS)

    if (PORTAL_PROFILE)
        target_compile_definitions(portal-core PUBLIC MI_TRACK_ETW=1)
    endif ()
elseif (APPLE)
    set(PORTAL_PLATFORM_MACOS ON)
    set(PORTAL_PLATFORM MACOS)

    target_compile_options(portal-core PUBLIC
            -fexperimental-library
            -stdlib=libc++
    )
    if (PORTAL_PROFILE)
        target_compile_definitions(portal-core PUBLIC MI_TRACK_VALGRIND=1)
    endif ()

    find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
    target_link_libraries(portal-core PUBLIC ${COREFOUNDATION_LIBRARY})
elseif (UNIX)
    set(PORTAL_PLATFORM_LINUX ON)
    set(PORTAL_PLATFORM LINUX)

    if (PORTAL_PROFILE)
        target_compile_definitions(portal-core PUBLIC MI_TRACK_VALGRIND=1)
    endif ()
else ()
    message(FATAL_ERROR "Unsupported platform")
endif ()

target_compile_definitions(portal-core
        PUBLIC
            $<BUILD_INTERFACE:$<$<CONFIG:Debug>:PORTAL_DEBUG>>
            $<BUILD_INTERFACE:$<$<CONFIG:RelWithDebInfo>:PORTAL_DEVELOPMENT>>
            $<BUILD_INTERFACE:$<$<CONFIG:Release>:PORTAL_DIST>>
        INTERFACE
            $<INSTALL_INTERFACE:$<$<CONFIG:Debug>:PORTAL_DEBUG>>
            $<INSTALL_INTERFACE:$<$<CONFIG:RelWithDebInfo>:PORTAL_DEVELOPMENT>>
            $<INSTALL_INTERFACE:$<$<CONFIG:Release>:PORTAL_DIST>>
)

if (PORTAL_PROFILE)
    target_compile_definitions(portal-core PUBLIC $<$<AND:$<CONFIG:RelWithDebInfo>,$<NOT:$<CONFIG:Release>>>:TRACY_ENABLE>)
    target_link_libraries(portal-core PUBLIC $<$<NOT:$<CONFIG:Release>>:Tracy::TracyClient>)
endif ()

if (MSVC)
    target_compile_options(portal-core PUBLIC
            $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>
            $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>
            $<$<CXX_COMPILER_ID:MSVC>:/permissive->
            $<$<CXX_COMPILER_ID:MSVC>:/volatile:iso>
            $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
            $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
    )
else ()
    target_compile_options(portal-core PUBLIC
            $<BUILD_INTERFACE:$<$<CONFIG:Release>:-g>>
            $<BUILD_INTERFACE:$<$<CXX_COMPILER_ID:GNU>:-Wno-missing-field-initializers>>
            $<BUILD_INTERFACE:$<$<C_COMPILER_ID:GNU>:-Wno-missing-field-initializers>>
    )
endif ()

portal_configure_pch(portal-core portal/core/config.h.inc CONFIGURE)

enable_testing()
portal_build_tests(tests)

portal_install_module(core
        FILES
        cmake/portal-test-helpers.cmake
        cmake/portal-install-helpers.cmake
        cmake/portal-module-helpers.cmake
)