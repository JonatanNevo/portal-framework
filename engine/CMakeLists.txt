cmake_minimum_required(VERSION 3.30)

# Experimental packaging tools
set(CMAKE_EXPERIMENTAL_EXPORT_PACKAGE_DEPENDENCIES 1942b4fa-b2c5-4546-9385-83f254070067)

file(READ "version.txt" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(portal-engine VERSION ${PROJECT_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} - version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS "portal/engine/*.cpp")
file(GLOB_RECURSE ENGINE_HEADER CONFIGURE_DEPENDS "portal/engine/*.h")

option(PORTAL_BUILD_TESTS "Whether or not to build the tests" ON)

find_package(Vulkan REQUIRED)
find_package(concurrentqueue CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(fastgltf CONFIG REQUIRED)
find_package(SPIRV-Tools CONFIG REQUIRED)
find_package(slang CONFIG REQUIRED)
find_package(glaze CONFIG REQUIRED)

option(PORTAL_FIND_PACAKGE "Whether or not to look for portal components" OFF) #OFF by default
if(PORTAL_FIND_PACAKGE)
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
        portal::core
        portal::application
        portal::input
        portal::networking
        portal::serialization
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
        concurrentqueue::concurrentqueue
        imgui::imgui
        fastgltf::fastgltf
        slang::slang
        SPIRV-Tools-static
        glaze::glaze
)

if(MSVC)
    target_compile_options(portal-engine PRIVATE /W4 /WX)
else()
    target_compile_options(portal-engine PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-missing-designated-field-initializers)
endif()

if(PORTAL_BUILD_TESTS)
    add_subdirectory(tests)
endif ()
unset(PORTAL_BUILD_TESTS CACHE)

target_compile_definitions(portal-engine PUBLIC
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
        VK_ENABLE_BETA_EXTENSIONS
)

find_program(SLANG_COMPILER "slangc" REQUIRED)

add_custom_target(compile-shaders ALL DEPENDS ${COMPILED_SHADERS})
add_dependencies(portal-engine compile-shaders)

foreach (BUILD_TYPE ${CMAKE_CONFIGURATION_TYPES})
    file(GLOB_RECURSE RESOURCES CONFIGURE_DEPENDS "resources/*")
    file(COPY ${RESOURCES} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${BUILD_TYPE}/resources")
endforeach ()

install(
        TARGETS portal-engine
        EXPORT portal-engine-targets
        FILE_SET HEADERS
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/portal-engine-version.cmake"
        COMPATIBILITY ExactVersion
)

install(
        EXPORT portal-engine-targets
        FILE portal-engine-config.cmake
        NAMESPACE portal::
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
        EXPORT_PACKAGE_DEPENDENCIES
)

install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/portal-engine-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/portal-engine-version.cmake"
        DESTINATION share/portal-engine
)

add_executable(engine_test main.cpp)
target_link_libraries(engine_test portal::engine)