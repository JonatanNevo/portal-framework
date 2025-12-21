cmake_minimum_required(VERSION 3.30)

file(READ "version.txt" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(portal-engine VERSION ${PROJECT_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} - version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS "portal/engine/*.cpp")
file(GLOB_RECURSE ENGINE_HEADERS CONFIGURE_DEPENDS "portal/engine/*.h")

option(PORTAL_BUILD_TESTS "Whether or not to build the tests" ON)
option(PORTAL_FIND_PACKAGE "Whether or not to look for portal components" OFF)

portal_add_module(engine
        SOURCES ${ENGINE_SOURCES}
        HEADERS ${ENGINE_HEADERS}

        PORTAL_DEPENDENCIES
            input
            networking
            serialization

        DEPENDENCIES
            imgui
            fastgltf
            slang
            glaze
            EnTT

        COMPLEX_DEPENDENCIES
            Vulkan|FIND_PACKAGE_ARGS|REQUIRED
            VulkanMemoryAllocator|LINK|GPUOpen::VulkanMemoryAllocator
)


if(PORTAL_BUILD_TESTS)
    add_subdirectory(tests)

    add_executable(engine_test MACOSX_BUNDLE main.cpp)
    target_link_libraries(engine_test portal::engine)

    set_target_properties(engine_test PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_IDENTIFIER com.portal.engine_test
            MACOSX_BUNDLE_BUNDLE_NAME "Engine Test"
            MACOSX_BUNDLE_ICON_FILE resources/portal_icon_64x64.icns
            MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
            MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
endif ()
unset(PORTAL_BUILD_TESTS CACHE)

target_compile_definitions(portal-engine PUBLIC
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
        VK_ENABLE_BETA_EXTENSIONS
)

foreach (BUILD_TYPE ${CMAKE_CONFIGURATION_TYPES})
    file(GLOB_RECURSE RESOURCES CONFIGURE_DEPENDS "resources/*")
    file(COPY ${RESOURCES} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${BUILD_TYPE}/resources")
endforeach ()

portal_install_module(engine)