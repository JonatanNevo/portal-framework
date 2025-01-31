file(GLOB_RECURSE NETWORKING_SOURCES CONFIGURE_DEPENDS "portal/networking/*.cpp")
file(GLOB_RECURSE NETWORKING_HEADERS CONFIGURE_DEPENDS "portal/networking/*.h")

find_package(GameNetworkingSockets CONFIG REQUIRED)

add_library(portal_networking STATIC ${NETWORKING_SOURCES})
add_library(portal::networking ALIAS portal_networking)

target_sources(portal_networking PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${NETWORKING_HEADERS}
)

target_link_libraries(portal_networking
        PUBLIC
        portal::core
        GameNetworkingSockets::shared
)