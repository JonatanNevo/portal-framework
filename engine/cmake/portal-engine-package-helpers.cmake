function(portal_game_configure_installer TARGET_NAME)
    set(options "")
    set(oneValueArgs
            URL
    )
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    get_target_property(DISPLAY_NAME ${TARGET_NAME} PORTAL_DISPLAY_NAME)

    if (NOT ARG_URL)
        set(ARG_URL "https://github.com/JonatanNevo/portal-framework")
    endif ()

    # set some IFW specific variables, which can be derived from the more generic variables given above
    set(CPACK_IFW_VERBOSE ON)
    set(CPACK_IFW_PACKAGE_TITLE ${DISPLAY_NAME})
    set(CPACK_IFW_PRODUCT_URL ${ARG_URL})

    # create a more memorable name for the maintenance tool (used for uninstalling the package)
    set(CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_NAME ${CMAKE_PROJECT_NAME}_MaintenanceTool)
    set(CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_INI_FILE ${CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_NAME}.ini)

    set(CPACK_IFW_PACKAGE_WIZARD_STYLE "Modern")

    get_target_property(WINDOWS_ICON ${TARGET_NAME} PORTAL_WINDOWS_ICON)
    get_target_property(MACOS_ICON ${TARGET_NAME} PORTAL_MACOS_ICON)
    get_target_property(LOGO ${TARGET_NAME} PORTAL_LOGO)

    if (WIN32)
        set(CPACK_IFW_PACKAGE_ICON ${WINDOWS_ICON})
    elseif (APPLE)
        set(CPACK_IFW_PACKAGE_ICON ${MACOS_ICON})
    endif ()
    set(CPACK_IFW_PACKAGE_WINDOWS_LOGO ${LOGO})
    set(CPACK_IFW_PACKAGE_LOGO ${LOGO})


    include(CPackIFW)
endfunction()
