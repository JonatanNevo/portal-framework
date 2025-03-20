cmake_minimum_required(VERSION 3.30)
project(portal-application)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB APP_SOURCES CONFIGURE_DEPENDS "portal/application/*.cpp")
file(GLOB APP_HEADERS CONFIGURE_DEPENDS "portal/application/*.h")
#file(GLOB UI_SOURCES CONFIGURE_DEPENDS "portal/application/ui/*.cpp")
#file(GLOB UI_HEADERS CONFIGURE_DEPENDS "portal/application/ui/*.h")
file(GLOB_RECURSE MODULE_SOURCES CONFIGURE_DEPENDS "portal/application/module/*.cpp")
file(GLOB_RECURSE MODULE_HEADERS CONFIGURE_DEPENDS "portal/application/module/*.h")
file(GLOB_RECURSE DEBUG_SOURCES CONFIGURE_DEPENDS "portal/application/debug/*.cpp")
file(GLOB_RECURSE DEBUG_HEADERS CONFIGURE_DEPENDS "portal/application/debug/*.h")
file(GLOB_RECURSE WINDOW_SOURCES CONFIGURE_DEPENDS "portal/application/window/*.cpp")
file(GLOB_RECURSE WINDOW_HEADERS CONFIGURE_DEPENDS "portal/application/window/*.h")

file(GLOB_RECURSE VULKAN_SOURCES CONFIGURE_DEPENDS "portal/application/vulkan/*.cpp")
file(GLOB_RECURSE VULKAN_HEADERS CONFIGURE_DEPENDS "portal/application/vulkan/*.h")

file(GLOB PLATFORM_SOURCES CONFIGURE_DEPENDS "portal/application/platform/*.cpp")
file(GLOB PLATFORM_HEADERS CONFIGURE_DEPENDS "portal/application/platform/*.h")
file(GLOB_RECURSE WINDOWS_SOURCES CONFIGURE_DEPENDS "portal/application/platform/windows/*.cpp")
file(GLOB_RECURSE WINDOWS_HEADERS CONFIGURE_DEPENDS "portal/application/platform/windows/*.h")
file(GLOB_RECURSE MAC_SOURCES CONFIGURE_DEPENDS "portal/application/platform/macos/*.cpp")
file(GLOB_RECURSE MAC_HEADERS CONFIGURE_DEPENDS "portal/application/platform/macos/*.h")
file(GLOB_RECURSE LINUX_SOURCES CONFIGURE_DEPENDS "portal/application/platform/linux/*.cpp")
file(GLOB_RECURSE LINUX_HEADERS CONFIGURE_DEPENDS "portal/application/platform/linux/*.h")

find_package(Vulkan REQUIRED COMPONENTS glslangValidator)
find_package(imgui CONFIG REQUIRED)
find_package(volk CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(unofficial-vulkan-memory-allocator-hpp CONFIG REQUIRED)
find_package(unofficial-shaderc CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)

option(PORTAL_FIND_PACAKGE "Whether or not to look for portal components" OFF) #OFF by default
if (PORTAL_FIND_PACAKGE)
    find_package(portal-core CONFIG REQUIRED)
    find_package(portal-serialization CONFIG REQUIRED)
endif ()
unset(PORTAL_FIND_PACAKGE CACHE)


if (PORTAL_PLATFORM_WINDOWS)
    set(PLATFORM_SOURCES ${WINDOWS_SOURCES} ${PLATFORM_SOURCES})
    set(PLATFORM_HEADERS ${WINDOWS_HEADERS} ${PLATFORM_HEADERS})
elseif (PORTAL_PLATFORM_MACOS)
    set(PLATFORM_SOURCES ${MAC_SOURCES} ${PLATFORM_SOURCES})
    set(PLATFORM_HEADERS ${MAC_HEADERS} ${PLATFORM_HEADERS})
else ()
    set(PLATFORM_SOURCES ${LINUX_SOURCES} ${PLATFORM_SOURCES}
            dummy.cpp
    )
    set(PLATFORM_HEADERS ${LINUX_HEADERS} ${PLATFORM_HEADERS})
endif ()

add_library(portal-application STATIC ${APP_SOURCES} ${PLATFORM_SOURCES} ${WINDOW_SOURCES} ${MODULE_SOURCES} ${DEBUG_SOURCES} ${VULKAN_SOURCES})
target_sources(portal-application PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${APP_HEADERS} ${PLATFORM_HEADERS} ${WINDOW_HEADERS} ${MODULE_HEADERS} ${DEBUG_HEADERS} ${VULKAN_HEADERS}
)

set_target_properties(portal-application PROPERTIES EXPORT_NAME application)

add_library(portal::application ALIAS portal-application)

target_link_libraries(portal-application
        PUBLIC
        portal::core
        portal::serialization
        imgui::imgui
        Vulkan::Vulkan
        unofficial::shaderc::shaderc
        glfw
        volk::volk
        GPUOpen::VulkanMemoryAllocator
        unofficial::VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
        spirv-cross-glsl
)

target_include_directories(portal-application PRIVATE ${Stb_INCLUDE_DIR}  ${Vulkan_INCLUDE_DIRS})
target_compile_definitions(portal-application PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 VK_NO_PROTOTYPES)

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