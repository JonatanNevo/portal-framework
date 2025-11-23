cmake_minimum_required(VERSION 3.30)

file(READ "version.txt" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(portal-engine VERSION ${PROJECT_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} - version: ${PROJECT_VERSION}")

include(../cmake/development.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS "portal/engine/*.cpp")
file(GLOB_RECURSE ENGINE_HEADER CONFIGURE_DEPENDS "portal/engine/*.h")

option(PORTAL_BUILD_TESTS "Whether or not to build the tests" ON)

find_package(Vulkan REQUIRED)
find_package(concurrentqueue CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(Ktx CONFIG REQUIRED)
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
        tinyobjloader::tinyobjloader
        concurrentqueue::concurrentqueue
        KTX::ktx
        imgui::imgui
        fastgltf::fastgltf
        slang::slang
        SPIRV-Tools-static
        glaze::glaze
)

prepare_consteval_strings(portal-engine ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_BINARY_DIR}/engine_map.inc)

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

file(GLOB_RECURSE SHADERS CONFIGURE_DEPENDS "shaders/*.shading.slang")
file(GLOB_RECURSE COMPUTE_SHADERS CONFIGURE_DEPENDS "shaders/*.compute.slang")
#foreach (SHADER ${SHADERS})
#    get_filename_component(SHADER_NAME ${SHADER} NAME)
#    get_filename_component(SHADER_DIR ${SHADER} DIRECTORY)
#    add_custom_command(
#            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}"
#            COMMAND ${SLANG_COMPILER} "${SHADER}" -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry vert_main -entry frag_main -o "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.spv"
#            DEPENDS "${SHADER}"
#            WORKING_DIRECTORY "${SHADER_DIR}"
#            COMMENT "Compiling shader: ${SHADER_NAME} to ${RESOURCES_OUTPUT_FOLDER}/${SHADER_NAME}.spv"
#    )
#    list(APPEND COMPILED_SHADERS "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}")
#endforeach ()
#foreach (SHADER ${COMPUTE_SHADERS})
#    get_filename_component(SHADER_NAME ${SHADER} NAME)
#    get_filename_component(SHADER_DIR ${SHADER} DIRECTORY)
#    add_custom_command(
#            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}"
#            COMMAND ${SLANG_COMPILER} "${SHADER}" -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry compute_main -o "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.spv"
#            DEPENDS "${SHADER}"
#            WORKING_DIRECTORY "${SHADER_DIR}"
#            COMMENT "Compiling shader: ${SHADER_NAME} to ${RESOURCES_OUTPUT_FOLDER}/${SHADER_NAME}.spv"
#    )
#    list(APPEND COMPILED_SHADERS "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}")
#endforeach ()

add_custom_target(compile-shaders ALL DEPENDS ${COMPILED_SHADERS})
add_dependencies(portal-engine compile-shaders)

foreach (BUILD_TYPE ${CMAKE_CONFIGURATION_TYPES})
    file(GLOB_RECURSE RESOURCES CONFIGURE_DEPENDS "resources/*")
    file(COPY ${RESOURCES} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${BUILD_TYPE}/resources")
endforeach ()
#
#
#install(TARGETS portal-engine
#        EXPORT portal-engine-targets
#        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
#        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
#        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
#        FILE_SET HEADERS DESTINATION include
#)
#
## Generate and install the export targets
#install(EXPORT portal-engine-targets
#        FILE portal-engine-targets.cmake
#        NAMESPACE portal::
#        DESTINATION share/portal-engine
#)
#
## Create and install the config file
#include(CMakePackageConfigHelpers)
#write_basic_package_version_file(
#        "${CMAKE_CURRENT_BINARY_DIR}/portal-engine-config-version.cmake"
#        VERSION ${PROJECT_VERSION}
#        COMPATIBILITY SameMajorVersion
#)
## To free it up for other add_subdirectories
#unset(PROJECT_VERSION)
#
#configure_package_config_file(
#        "../cmake/portal-engine-config.cmake.in"
#        "${CMAKE_CURRENT_BINARY_DIR}/portal-engine-config.cmake"
#        INSTALL_DESTINATION share/portal-engine
#)
#
## Install the config files
#install(FILES
#        "${CMAKE_CURRENT_BINARY_DIR}/portal-engine-config.cmake"
#        "${CMAKE_CURRENT_BINARY_DIR}/portal-engine-config-version.cmake"
#        DESTINATION share/portal-engine
#)
#
add_executable(engine-test main.cpp)
target_link_libraries(engine-test
        PUBLIC
        portal::engine
)
