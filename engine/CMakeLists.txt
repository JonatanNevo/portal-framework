cmake_minimum_required(VERSION 3.30)

file(READ "version.txt" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(portal-engine VERSION ${PROJECT_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} - version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/portal-engine-helpers.cmake)
include(cmake/portal-engine-package-helpers.cmake)

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS "portal/engine/*.cpp")
file(GLOB_RECURSE ENGINE_HEADERS CONFIGURE_DEPENDS "portal/engine/*.h")

option(PORTAL_BUILD_TESTS "Whether or not to build the tests" ON)
option(PORTAL_FIND_PACKAGE "Whether or not to look for portal components" OFF)

if (PORTAL_FIND_PACKAGE)
    find_package(portal-core CONFIG REQUIRED)
endif ()

portal_add_module(engine
        SOURCES ${ENGINE_SOURCES}
        HEADERS ${ENGINE_HEADERS}

        PORTAL_DEPENDENCIES
            input
            networking
            serialization

        PORTAL_FIND_PACKAGE ${PORTAL_FIND_PACKAGE}

        DEPENDENCIES
            imgui
            fastgltf
            slang
            glaze
            EnTT
            volk
            imguizmo

        COMPLEX_DEPENDENCIES
            VulkanHeaders|LINK|Vulkan::Headers
            VulkanMemoryAllocator|LINK|GPUOpen::VulkanMemoryAllocator

        COMPILE_CONFIG_FILE
            portal/engine/config.h
)
portal_add_resources(portal-engine
        resources
        OUTPUT_NAME
            engine
)

set_target_properties(portal-engine PROPERTIES PORTAL_ENGINE_VERSION ${PROJECT_VERSION})
set_property(TARGET ${TARGET_NAME} APPEND PROPERTY EXPORT_PROPERTIES PORTAL_ENGINE_VERSION)

#enable_testing()
#portal_build_tests(tests)

target_compile_definitions(portal-engine PUBLIC
        VK_NO_PROTOTYPES
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
        VK_ENABLE_BETA_EXTENSIONS
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        IMGUI_IMPL_VULKAN_USE_VOLK
)

portal_install_module(engine
        FILES
        cmake/portal-engine-helpers.cmake
        cmake/portal-engine-package-helpers.cmake
)