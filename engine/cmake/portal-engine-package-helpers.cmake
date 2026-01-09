function(portal_game_configure_installer TARGET_NAME)
    set(options "")
    set(oneValueArgs
            DESCRIPTION
            VENDOR
            CONTACT
            URL
    )
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    get_target_property(DISPLAY_NAME ${TARGET_NAME} PORTAL_DISPLAY_NAME)

    if (NOT ARG_URL)
        set(ARG_URL "https://github.com/JonatanNevo/portal-framework")
    endif ()

    set(CPACK_PACKAGE_NAME ${DISPLAY_NAME})
    set(CPACK_PACKAGE_VENDOR ${ARG_VENDOR})
    set(CPACK_PACKAGE_CONTACT ${ARG_CONTACT})
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${ARG_DESCRIPTION})
    set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
    set(CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_PROJECT_VERSION_MAJOR})
    set(CPACK_PACKAGE_VERSION_MINOR ${CMAKE_PROJECT_VERSION_MINOR})
    set(CPACK_PACKAGE_VERSION_PATCH ${CMAKE_PROJECT_VERSION_PATCH})
    set(CPACK_VERBATIM_VARIABLES YES)

    set(CPACK_COMPONENTS_GROUPING IGNORE)

    # set some IFW specific variables, which can be derived from the more generic variables given above
    set(CPACK_IFW_VERBOSE ON)
    set(CPACK_IFW_PACKAGE_TITLE ${DISPLAY_NAME})
    set(CPACK_IFW_PACKAGE_PUBLISHER ${ARG_VENDOR})
    set(CPACK_IFW_PRODUCT_URL ${ARG_URL})

    # create a more memorable name for the maintenance tool (used for uninstalling the package)
    set(CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_NAME ${CMAKE_PROJECT_NAME}_MaintenanceTool)
    set(CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_INI_FILE ${CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_NAME}.ini)

    get_target_property(WINDOWS_ICON ${TARGET_NAME} PORTAL_WINDOWS_ICON)
    get_target_property(MACOS_ICON ${TARGET_NAME} PORTAL_MACOS_ICON)
    get_target_property(GENERIC_ICON ${TARGET_NAME} PORTAL_ICON)

    if (WIN32)
        set(CPACK_IFW_PACKAGE_ICON ${WINDOWS_ICON})
    elseif (APPLE)
        set(CPACK_IFW_PACKAGE_ICON ${MACOS_ICON})
    endif ()
    set(CPACK_IFW_PACKAGE_WINDOWS_LOGO ${GENERIC_ICON})
    set(CPACK_IFW_PACKAGE_LOGO ${GENERIC_ICON})


    include(CPack)
    include(CPackIFW)

    cpack_add_component(${TARGET_NAME}
            REQUIRED
            DISPLAY_NAME ${CMAKE_PROJECT_NAME}
            DESCRIPTION ${ARG_DESCRIPTION}
    )

endfunction()
