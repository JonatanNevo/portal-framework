function(portal_add_dependency MODULE_NAME DEPENDENCY)
    set(options PORTAL SKIP_LINK)
    set(oneValueArgs LINK)
    set(multiValueArgs FIND_PACKAGE_ARGS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT ARG_LINK)
        if (ARG_PORTAL)
            set(ARG_LINK portal::${DEPENDENCY})
        else ()
            set(ARG_LINK ${DEPENDENCY}::${DEPENDENCY})
        endif ()
    endif ()

    if (NOT ARG_FIND_PACKAGE_ARGS)
        set(ARG_FIND_PACKAGE_ARGS CONFIG REQUIRED)
    endif ()

    if (ARG_PORTAL)
        if(PORTAL_FIND_PACKAGE)
            find_package(portal-${DEPENDENCY} CONFIG REQUIRED)
        endif ()
    else ()
        find_package(${DEPENDENCY} ${ARG_FIND_PACKAGE_ARGS})
    endif ()

    if (NOT ARG_SKIP_LINK)
        target_link_libraries(portal-${MODULE_NAME} PUBLIC ${ARG_LINK})
    endif()
endfunction()

function(portal_add_module MODULE_NAME)
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs
            SOURCES
            HEADERS
            WINDOWS_SOURCES
            WINDOWS_HEADERS
            MACOS_SOURCES
            MACOS_HEADERS
            LINUX_SOURCES
            LINUX_HEADERS
            PORTAL_DEPENDENCIES
            DEPENDENCIES
            COMPLEX_DEPENDENCIES
    )
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(TARGET_NAME portal-${MODULE_NAME})
    message(STATUS "Adding module - ${TARGET_NAME}")

    add_library(${TARGET_NAME} STATIC ${ARG_SOURCES})
    target_sources(${TARGET_NAME} PUBLIC
            FILE_SET HEADERS
            BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
            FILES ${ARG_HEADERS}
    )
    set_target_properties(${TARGET_NAME} PROPERTIES EXPORT_NAME ${MODULE_NAME})
    add_library(portal::${MODULE_NAME} ALIAS ${TARGET_NAME})

    if (WIN32)
        if (ARG_WINDOWS_SOURCES OR ARG_WINDOWS_HEADERS)
            target_sources(${TARGET_NAME}
                    PRIVATE ${ARG_WINDOWS_SOURCES}
                    PUBLIC
                    FILE_SET HEADERS
                    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                    FILES ${ARG_WINDOWS_HEADERS}
            )
        endif ()
    elseif (APPLE)
        if (ARG_MACOS_SOURCES OR ARG_MACOS_HEADERS)
            target_sources(${TARGET_NAME}
                    PRIVATE ${ARG_MACOS_SOURCES}
                    PUBLIC
                    FILE_SET HEADERS
                    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                    FILES ${ARG_MACOS_HEADERS}
            )
        endif ()
    elseif (UNIX)
        if (ARG_LINUX_SOURCES OR ARG_LINUX_HEADERS)
            target_sources(${TARGET_NAME}
                    PRIVATE ${ARG_LINUX_SOURCES}
                    PUBLIC
                    FILE_SET HEADERS
                    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                    FILES ${ARG_LINUX_HEADERS}
            )
        endif ()
    else ()
        message(FATAL_ERROR "Unsupported platform")
    endif ()

    if(MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
    else()
        target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-missing-designated-field-initializers)
    endif()

    foreach(dep ${ARG_PORTAL_DEPENDENCIES})
        portal_add_dependency(${MODULE_NAME} ${dep} PORTAL)
    endforeach()

    foreach(dep ${ARG_DEPENDENCIES})
        portal_add_dependency(${MODULE_NAME} ${dep})
    endforeach()

    foreach(dep_spec ${ARG_COMPLEX_DEPENDENCIES})
        string(REPLACE "|" " " dep_args "${dep_spec}")
        separate_arguments(dep_args)
        list(GET dep_args 0 dep_name)
        list(REMOVE_AT dep_args 0)

        portal_add_dependency(${MODULE_NAME} ${dep_name} ${dep_args})
    endforeach()

    unset(PORTAL_FIND_PACKAGE CACHE)
endfunction()