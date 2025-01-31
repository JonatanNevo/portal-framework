file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "portal/core/*.cpp")
file(GLOB_RECURSE CORE_HEADER CONFIGURE_DEPENDS "portal/core/*.h")

find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_library(portal_core STATIC ${CORE_SOURCES})
add_library(portal::core ALIAS portal_core)

target_sources(portal_core PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${CORE_HEADER}
)

target_link_libraries(portal_core
        PUBLIC
        spdlog::spdlog
        glm::glm
)
