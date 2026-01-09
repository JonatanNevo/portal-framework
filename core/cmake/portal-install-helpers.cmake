function(portal_install_module MODULE_NAME)
    # Experimental packaging tools
    set(CMAKE_EXPERIMENTAL_EXPORT_PACKAGE_DEPENDENCIES 1942b4fa-b2c5-4546-9385-83f254070067)

    set(TARGET_NAME portal-${MODULE_NAME})

    set(options "")
    set(oneValueArgs COMPONENT)
    set(multiValueArgs FILES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT ARG_COMPONENT)
        # Replace hyphens with underscores for NSIS compatibility
        string(REPLACE "-" "_" ARG_COMPONENT ${TARGET_NAME})
    endif ()

    set(INSTALLATION_TARGET ${TARGET_NAME}-targets)
    set(VERSION_TARGET ${TARGET_NAME}-version)
    set(CONFIG_TARGET ${TARGET_NAME}-config)


    set (FILE_SETS HEADERS)
    get_target_property(HAS_ADDITIONAL_HEADERS ${TARGET_NAME} HEADER_SET_additional_headers)
    if (HAS_ADDITIONAL_HEADERS)
        install(
                TARGETS ${TARGET_NAME}
                EXPORT ${INSTALLATION_TARGET}
                COMPONENT ${ARG_COMPONENT}
                FILE_SET HEADERS
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
                FILE_SET additional_headers
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    else ()
        install(
                TARGETS ${TARGET_NAME}
                EXPORT ${INSTALLATION_TARGET}
                COMPONENT ${ARG_COMPONENT}
                FILE_SET HEADERS
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    endif ()

    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/${VERSION_TARGET}.cmake"
            COMPATIBILITY ExactVersion
    )

    install(
            EXPORT ${INSTALLATION_TARGET}
            FILE ${INSTALLATION_TARGET}.cmake
            COMPONENT ${ARG_COMPONENT}
            NAMESPACE portal::
            DESTINATION share/${TARGET_NAME}
            EXPORT_PACKAGE_DEPENDENCIES
    )

    set(CONFIG_CONTENT "@PACKAGE_INIT@\n\n")
    string(APPEND CONFIG_CONTENT "include(\"\${CMAKE_CURRENT_LIST_DIR}/${INSTALLATION_TARGET}.cmake\")\n")

    if (ARG_FILES)
        string(APPEND CONFIG_CONTENT "\n# Include helper scripts\n")
        foreach (FILE ${ARG_FILES})
            get_filename_component(FILE_NAME ${FILE} NAME)
            string(APPEND CONFIG_CONTENT "include(\"\${CMAKE_CURRENT_LIST_DIR}/${FILE_NAME}\")\n")
        endforeach ()
    endif ()

    string(APPEND CONFIG_CONTENT "\ncheck_required_components(${TARGET_NAME})\n")

    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_TARGET}.cmake.in" "${CONFIG_CONTENT}")

    configure_package_config_file(
            "${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_TARGET}.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_TARGET}.cmake"
            INSTALL_DESTINATION share/${TARGET_NAME}
    )

    set(FILES_TO_INSTALL
            "${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_TARGET}.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/${VERSION_TARGET}.cmake"
    )

    if (ARG_FILES)
        list(APPEND FILES_TO_INSTALL ${ARG_FILES})
    endif ()

    install(
            FILES ${FILES_TO_INSTALL}
            COMPONENT ${ARG_COMPONENT}
            DESTINATION share/${TARGET_NAME}
    )


    get_target_property(RESOURCES_FOLDERS ${TARGET_NAME} PORTAL_RESOURCES)
    if (NOT RESOURCES_FOLDERS OR RESOURCES_FOLDERS STREQUAL "PORTAL_RESOURCES-NOTFOUND")
        set(RESOURCES_FOLDERS "")
    endif ()

    foreach (RESOURCE_FOLDER IN LISTS RESOURCES_FOLDERS)
        install(
            DIRECTORY "$<TARGET_FILE_DIR:${TARGET_NAME}>/resources/${RESOURCE_FOLDER}"
            COMPONENT ${ARG_COMPONENT}
            DESTINATION resources
        )
    endforeach ()
endfunction()