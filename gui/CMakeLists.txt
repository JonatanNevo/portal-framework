cmake_minimum_required(VERSION 3.30)

file(READ "version.txt" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(portal-gui VERSION ${PROJECT_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} - version: ${PROJECT_VERSION}")

include(../cmake/development.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE GUI_SOURCES CONFIGURE_DEPENDS "portal/gui/*.cpp")
file(GLOB_RECURSE GUI_HEADERS CONFIGURE_DEPENDS "portal/gui/*.h")
file(GLOB_RECURSE GUI_UI_SOURCES CONFIGURE_DEPENDS "portal/gui/ui/*.cpp")
file(GLOB_RECURSE GUI_UI_HEADERS CONFIGURE_DEPENDS "portal/gui/ui/*.h")
file(GLOB GUI_TEXTURES CONFIGURE_DEPENDS "resources/textures/*")

find_package(imgui CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Stb REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

option(PORTAL_FIND_PACAKGE "Whether or not to look for portal components" OFF) #OFF by default
if (PORTAL_FIND_PACAKGE)
    find_package(portal-application CONFIG REQUIRED)
    find_package(portal-assets CONFIG REQUIRED)
    find_package(portal-input CONFIG REQUIRED)
endif ()
unset(PORTAL_FIND_PACAKGE CACHE)

add_library(portal-gui STATIC ${GUI_SOURCES} ${GUI_UI_SOURCES})

target_sources(portal-gui PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${GUI_HEADERS} ${GUI_UI_HEADERS}
)

# TODO: Replace this with an archive file and include it somehow in the install folder
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/resources/textures)
foreach(texture IN LISTS GUI_TEXTURES)
    get_filename_component(texture_name ${texture} NAME)
    file(COPY_FILE ${texture} ${CMAKE_BINARY_DIR}/resources/textures/${texture_name})
endforeach()

set_target_properties(portal-gui PROPERTIES EXPORT_NAME gui)

add_library(portal::gui ALIAS portal-gui)

target_link_libraries(portal-gui
        PUBLIC
        imgui::imgui
        Vulkan::Vulkan
        portal::application
        portal::assets
        portal::input
        glfw
)

target_include_directories(portal-gui PRIVATE ${Stb_INCLUDE_DIR})

if(MSVC)
    target_compile_options(portal-gui PRIVATE /W4 /WX)
else()
    target_compile_options(portal-gui PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-missing-designated-field-initializers)
endif()


install(TARGETS portal-gui
        EXPORT portal-gui-targets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        FILE_SET HEADERS DESTINATION include
)

# Generate and install the export targets
install(EXPORT portal-gui-targets
        FILE portal-gui-targets.cmake
        NAMESPACE portal::
        DESTINATION share/portal-gui
)

# Create and install the config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/portal-gui-config-version.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)
unset(PROJECT_VERSION)

configure_package_config_file(
        "../cmake/portal-gui-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/portal-gui-config.cmake"
        INSTALL_DESTINATION share/portal-gui
)

# Install the config files
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/portal-gui-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/portal-gui-config-version.cmake"
        DESTINATION share/portal-gui
)