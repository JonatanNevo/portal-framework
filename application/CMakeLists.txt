file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "portal/application/*.cpp")
file(GLOB_RECURSE APP_HEADERS CONFIGURE_DEPENDS "portal/application/*.h")
file(GLOB_RECURSE HEADLESS_SOURCES CONFIGURE_DEPENDS "portal/headless/*.cpp")
file(GLOB_RECURSE HEADLESS_HEADERS CONFIGURE_DEPENDS "portal/headless/*.h")
file(GLOB_RECURSE GUI_SOURCES CONFIGURE_DEPENDS "portal/gui/*.cpp")
file(GLOB_RECURSE GUI_HEADERS CONFIGURE_DEPENDS "portal/gui/*.h")
file(GLOB_RECURSE GUI_UI_SOURCES CONFIGURE_DEPENDS "portal/ui/*.cpp")
file(GLOB_RECURSE GUI_UI_HEADERS CONFIGURE_DEPENDS "portal/ui/*.h")

find_package(imgui CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Stb REQUIRED)
find_package(glfw3 CONFIG REQUIRED)


option(PORTAL_HEADLESS "Build headless application" OFF)
if (PORTAL_HEADLESS)
    add_library(portal_application STATIC ${APP_SOURCES} ${HEADLESS_SOURCES})
    target_sources(portal_application PUBLIC
            FILE_SET HEADERS
            BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
            FILES ${HEADLESS_HEADERS}
    )
    add_compile_definitions(PORTAL_HEADLESS)
else ()
    add_library(portal_application STATIC ${APP_SOURCES} ${GUI_SOURCES} ${GUI_UI_SOURCES})
    target_sources(portal_application PUBLIC
            FILE_SET HEADERS
            BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
            FILES ${GUI_HEADERS} ${GUI_UI_HEADERS}
    )
endif ()

add_library(portal::application ALIAS portal_application)

target_sources(portal_application PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${APP_HEADERS}
)

target_link_libraries(portal_application
        PUBLIC
        imgui::imgui
        Vulkan::Vulkan
        portal::core
        glfw
)

target_include_directories(portal_application PRIVATE ${Stb_INCLUDE_DIR})