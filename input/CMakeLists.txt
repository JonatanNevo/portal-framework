file(GLOB_RECURSE INPUT_SOURCES CONFIGURE_DEPENDS "portal/input/*.cpp")
file(GLOB_RECURSE INPUT_HEADERS CONFIGURE_DEPENDS "portal/input/*.h")

find_package(glfw3 CONFIG REQUIRED)

add_library(portal_input STATIC ${INPUT_SOURCES})
add_library(portal::input ALIAS portal_input)

target_sources(portal_input PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${INPUT_HEADERS}
)

target_link_libraries(portal_input
        PUBLIC
        portal::core
        glfw
)