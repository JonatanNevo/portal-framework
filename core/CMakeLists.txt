cmake_minimum_required(VERSION 3.30)

# Experimental packaging tools
set(CMAKE_EXPERIMENTAL_EXPORT_PACKAGE_DEPENDENCIES 1942b4fa-b2c5-4546-9385-83f254070067)

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
option(PORTAL_BUILD_DIST "Build the distribution package" OFF)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "portal/core/*.cpp")
file(GLOB_RECURSE CORE_HEADERS CONFIGURE_DEPENDS "portal/core/*.h")

file(GLOB_RECURSE CORE_HAL_HEADERS CONFIGURE_DEPENDS "portal/platform/core/hal/*.h")

file(GLOB_RECURSE CORE_WINDOWS_SOURCES CONFIGURE_DEPENDS "portal/platform/core/windows/*.cpp")
file(GLOB_RECURSE CORE_WINDOWS_HEADERS CONFIGURE_DEPENDS "portal/platform/core/windows/*.h")

file(GLOB_RECURSE CORE_MACOS_SOURCES CONFIGURE_DEPENDS "portal/platform/core/macos/*.cpp")
file(GLOB_RECURSE CORE_MACOS_HEADERS CONFIGURE_DEPENDS "portal/platform/core/macos/*.h")

file(GLOB_RECURSE CORE_LINUX_SOURCES CONFIGURE_DEPENDS "portal/platform/core/linux/*.cpp")
file(GLOB_RECURSE CORE_LINUX_HEADERS CONFIGURE_DEPENDS "portal/platform/core/linux/*.h")

find_package(llvm-adt CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(enchantum CONFIG REQUIRED)
find_package(mimalloc CONFIG REQUIRED)
find_package(frozen CONFIG REQUIRED)
find_package(concurrentqueue CONFIG REQUIRED)

if (PORTAL_PROFILE)
    set(CACHE TRACY_ENABLE TYPE BOOL FORCE ON)
    find_package(Tracy CONFIG REQUIRED)
else ()
    set(CACHE TRACY_ENABLE TYPE BOOL FORCE OFF)
endif ()

add_library(portal-core STATIC ${CORE_SOURCES})
add_library(portal::core ALIAS portal-core)

target_sources(portal-core PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${CORE_HEADERS} ${CORE_HAL_HEADERS}
)

set_target_properties(portal-core PROPERTIES EXPORT_NAME core)
target_link_libraries(portal-core
        PUBLIC
        llvm::adt
        glm::glm
        spdlog::spdlog
        enchantum::enchantum
        mimalloc-static
        frozen::frozen
        concurrentqueue::concurrentqueue
)

target_compile_definitions(portal-core PUBLIC
        GLM_FORCE_SWIZZLE
        GLM_FORCE_RADIANS
        GLM_FORCE_CTOR_INIT
        GLM_ENABLE_EXPERIMENTAL
        GLM_FORCE_CXX20
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        SPDLOG_USE_STD_FORMAT
)

if (WIN32)
    target_sources(portal-core
            PRIVATE ${CORE_WINDOWS_SOURCES}
            PUBLIC
            FILE_SET HEADERS
            BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
            FILES ${CORE_WINDOWS_HEADERS}
    )
elseif (APPLE)
    target_sources(portal-core
            PRIVATE ${CORE_MACOS_SOURCES}
            PUBLIC
            FILE_SET HEADERS
            BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
            FILES ${CORE_MACOS_HEADERS}
    )
elseif (UNIX)
    message(STATUS ${CORE_LINUX_SOURCES})
    target_sources(portal-core
            PRIVATE ${CORE_LINUX_SOURCES}
            PUBLIC
            FILE_SET HEADERS
            BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
            FILES ${CORE_LINUX_HEADERS}
    )
else ()
    message(FATAL_ERROR "Unsupported platform")
endif ()

if (WIN32)
    set(PORTAL_PLATFORM_WINDOWS ON)
    set(PORTAL_PLATFORM WINDOWS)

    if (PORTAL_PROFILE)
        target_compile_definitions(portal-core PUBLIC MI_TRACK_ETW=1)
    endif ()
elseif (APPLE)
    set(PORTAL_PLATFORM_MACOS ON)
    set(PORTAL_PLATFORM MACOS)

    target_compile_options(portal-core PUBLIC -fexperimental-library -stdlib=libc++)
    if (PORTAL_PROFILE)
        target_compile_definitions(portal-core PUBLIC MI_TRACK_VALGRIND=1)
    endif ()
elseif (UNIX)
    set(PORTAL_PLATFORM_LINUX ON)
    set(PORTAL_PLATFORM LINUX)

    if (PORTAL_PROFILE)
        target_compile_definitions(portal-core PUBLIC MI_TRACK_VALGRIND=1)
    endif ()
else ()
    message(FATAL_ERROR "Unsupported platform")
endif ()

if (NOT PORTAL_BUILD_DIST)
    target_compile_definitions(portal-core PUBLIC
            $<$<CONFIG:Debug>:PORTAL_DEBUG>
            $<$<CONFIG:RelWithDebInfo>:PORTAL_DEVELOPMENT>
            $<$<CONFIG:Release>:PORTAL_RELEASE>
    )
else ()
    target_compile_definitions(portal-core PUBLIC PORTAL_DIST)
endif ()

if (PORTAL_PROFILE AND NOT PORTAL_BUILD_DIST)
    target_compile_definitions(portal-core PUBLIC $<$<CONFIG:RelWithDebInfo>:TRACY_ENABLE>)
    target_link_libraries(portal-core PUBLIC Tracy::TracyClient)
endif ()

if (MSVC)
    target_compile_options(portal-core PUBLIC
            "/Zc:__cplusplus"
            "/Zc:preprocessor"
            "/permissive-"
            "/volatile:iso"
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
    target_compile_options(portal-core PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-missing-designated-field-initializers)
endif ()

configure_file("portal/core/config.h.cmake" "portal/core/config.h")
target_precompile_headers(portal-core PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/portal/core/config.h)

if (PORTAL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()
unset(PORTAL_BUILD_TESTS CACHE)

install(
    TARGETS portal-core
    EXPORT portal-core-targets
    FILE_SET HEADERS
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/portal-core-version.cmake"
    COMPATIBILITY ExactVersion
)

install(
    EXPORT portal-core-targets
    FILE portal-core-config.cmake
    NAMESPACE portal::
    DESTINATION share/portal-core
    EXPORT_PACKAGE_DEPENDENCIES
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/portal-core-version.cmake"
    DESTINATION share/portal-core
)