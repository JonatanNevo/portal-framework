function(portal_add_resources TARGET_NAME RESOURCE_PATH)
    set(options "")
    set(oneValueArgs OUTPUT_NAME)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    get_filename_component(RESOURCE_DIR_NAME "${RESOURCE_PATH}" NAME)

    if (NOT ARG_OUTPUT_NAME)
        set(ARG_OUTPUT_NAME ${RESOURCE_DIR_NAME})
    endif ()

    file(GLOB_RECURSE RESOURCE_FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCE_PATH}/*"
    )

    set(RESOURCE_TARGET_NAME "${TARGET_NAME}_copy_${ARG_OUTPUT_NAME}")

    add_custom_target(${RESOURCE_TARGET_NAME}
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCE_PATH}"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/resources/${ARG_OUTPUT_NAME}"
            DEPENDS ${RESOURCE_FILES}
            COMMENT "Copying resources for ${TARGET_NAME}: ${RESOURCE_PATH} -> resources/${ARG_OUTPUT_NAME}"
            VERBATIM
    )

    add_dependencies(${TARGET_NAME} ${RESOURCE_TARGET_NAME})

    get_target_property(EXISTING_RESOURCES ${TARGET_NAME} PORTAL_RESOURCES)
    if(NOT EXISTING_RESOURCES)
        set(EXISTING_RESOURCES "")
    endif()

    list(APPEND EXISTING_RESOURCES "${ARG_OUTPUT_NAME}")
    set_target_properties(${TARGET_NAME} PROPERTIES PORTAL_RESOURCES "${EXISTING_RESOURCES}")
endfunction()

function(portal_fetch_resources TARGET_NAME TARGET_TO_FETCH)
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs RESOURCES_TO_FETCH)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    get_target_property(TARGET_RESOURCES ${TARGET_TO_FETCH} PORTAL_RESOURCES)
    if (NOT TARGET_RESOURCES)
        message(FATAL_ERROR "Failed to fetch resources from target ${TARGET_TO_FETCH}")
    endif ()

    get_target_property(EXISTING_ADDITIONAL_RESOURCES ${TARGET_NAME} PORTAL_ADDITIONAL_RESOURCES)
    if(NOT EXISTING_ADDITIONAL_RESOURCES)
        set(EXISTING_ADDITIONAL_RESOURCES "")
    endif()

    set(RESOURCES_TO_COPY ${TARGET_RESOURCES})
    if (ARG_RESOURCES_TO_FETCH)
        # Validate and use only specific resources
        set(RESOURCES_TO_COPY "")
        foreach (RESOURCE_PATH ${ARG_RESOURCES_TO_FETCH})
            if(NOT RESOURCE_PATH IN_LIST TARGET_RESOURCES)
                message(FATAL_ERROR "Resource '${RESOURCE_PATH}' not found in target ${TARGET_TO_FETCH}. Available resources: ${TARGET_RESOURCES}")
            endif()
            list(APPEND RESOURCES_TO_COPY "${RESOURCE_PATH}")
        endforeach ()
    endif ()

    foreach (RESOURCE_PATH ${RESOURCES_TO_COPY})
        list(APPEND EXISTING_ADDITIONAL_RESOURCES "${RESOURCE_PATH}")

        set(FETCH_TARGET_NAME "${TARGET_NAME}_fetch_${RESOURCE_PATH}_from_${TARGET_TO_FETCH}")

        set(SOURCE_COPY_TARGET "${TARGET_TO_FETCH}_copy_${RESOURCE_PATH}")

        add_custom_target(${FETCH_TARGET_NAME}
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "$<TARGET_FILE_DIR:${TARGET_TO_FETCH}>/resources/${RESOURCE_PATH}"
                "$<TARGET_FILE_DIR:${TARGET_NAME}>/resources/${RESOURCE_PATH}"
                COMMENT "Fetching resources for ${TARGET_NAME}: ${TARGET_TO_FETCH}/resources/${RESOURCE_PATH} -> resources/${RESOURCE_PATH}"
                VERBATIM
        )

        if(TARGET ${SOURCE_COPY_TARGET})
            add_dependencies(${FETCH_TARGET_NAME} ${SOURCE_COPY_TARGET})
        endif()
        add_dependencies(${FETCH_TARGET_NAME} ${TARGET_TO_FETCH})

        add_dependencies(${TARGET_NAME} ${FETCH_TARGET_NAME})
    endforeach ()

    set_target_properties(${TARGET_NAME} PROPERTIES PORTAL_ADDITIONAL_RESOURCES "${EXISTING_ADDITIONAL_RESOURCES}")
endfunction()

function(portal_read_settings TARGET_NAME)
    get_target_property(SETTINGS_PATH ${TARGET_NAME} PORTAL_SETTINGS_PATH)

    if (NOT EXISTS ${SETTINGS_PATH})
        message(FATAL_ERROR "Failed to find settings in ${SETTINGS_PATH}")
    endif ()

    # TODO: validate engine version matches what in the json

    cmake_path(GET SETTINGS_PATH FILENAME SETTINGS_FILENAME)
    set(SETTINGS_OUTPUT_PATH "$<TARGET_FILE_DIR:${TARGET_NAME}>/${SETTINGS_FILENAME}")

    set(SETTINGS_COPY_TARGET_NAME "${TARGET_NAME}_copy_settings")
    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SETTINGS_FILENAME}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SETTINGS_PATH}"
            "${SETTINGS_OUTPUT_PATH}"
            DEPENDS ${SETTINGS_PATH}
            COMMENT "Copying settings file for ${TARGET_NAME}: ${SETTINGS_FILENAME}"
            VERBATIM
    )
    add_custom_target(${SETTINGS_COPY_TARGET_NAME} DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${SETTINGS_FILENAME})
    add_dependencies(${TARGET_NAME} ${SETTINGS_COPY_TARGET_NAME})

    file(READ ${SETTINGS_PATH} SETTINGS_CONTENT)
    string(JSON RESOURCES_LIST_LENGTH LENGTH ${SETTINGS_CONTENT} engine resources)
    if(RESOURCES_LIST_LENGTH GREATER 0)
        math(EXPR RESOURCES_LIST_LAST_INDEX "${RESOURCES_LIST_LENGTH} - 1")
        foreach(IDX RANGE ${RESOURCES_LIST_LAST_INDEX})
            string(JSON RESOURCE_OBJECT GET "${SETTINGS_CONTENT}" engine resources ${IDX})
            string(JSON RESOURCE_TYPE GET ${RESOURCE_OBJECT} type)
            string(JSON RESOURCE_PATH GET ${RESOURCE_OBJECT} path)

            cmake_path(IS_RELATIVE RESOURCE_PATH IS_RELATIVE_VAR)

            if (IS_RELATIVE_VAR)
                file(GLOB_RECURSE RESOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCE_PATH}/*")
                set(RESOURCE_TARGET_NAME "${TARGET_NAME}_copy_settings_${RESOURCE_PATH}")

                add_custom_target(${RESOURCE_TARGET_NAME}
                        COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCE_PATH}"
                        "$<TARGET_FILE_DIR:${TARGET_NAME}>/resources/${RESOURCE_PATH}"
                        DEPENDS ${RESOURCE_FILES} ${SETTINGS_PATH}
                        COMMENT "Copying resources form ${TARGET_NAME} settings: ${RESOURCE_PATH} -> resources/${ARG_OUTPUT_NAME}"
                        VERBATIM
                )

                add_dependencies(${TARGET_NAME} ${RESOURCE_TARGET_NAME})
            else ()
                message(STATUS "Found absolute resource path, skipping... ${RESOURCE_PATH}")
            endif ()
        endforeach ()
    endif()
endfunction()

function(portal_add_game TARGET_NAME)
    set(options "")
    set(oneValueArgs SETTINGS_FILE)
    set(multiValueArgs
            SOURCES
            RESOURCE_PATHS
            LINK_LIBRARIES
    )
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # TODO: add editor target
    if (APPLE)
        # TODO: get these as arguments
        add_executable(${TARGET_NAME} MACOSX_BUNDLE ${ARG_SOURCES})
        set_target_properties(${TARGET_NAME} PROPERTIES
                MACOSX_BUNDLE TRUE
                MACOSX_BUNDLE_IDENTIFIER com.portal.${TARGET_NAME}
                MACOSX_BUNDLE_BUNDLE_NAME "Engine Test"
                MACOSX_BUNDLE_ICON_FILE resources/portal_icon_64x64.icns
                MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
                MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        )
    else()
        add_executable(${TARGET_NAME} ${ARG_SOURCES})
    endif ()

    target_link_libraries(
            ${TARGET_NAME}
            portal::engine
            ${ARG_LINK_LIBRARIES}
    )

    portal_fetch_resources(${TARGET_NAME} portal-engine)

    foreach (RESOURCE_PATH ${ARG_RESOURCE_PATHS})
        portal_add_resources(${TARGET_NAME} ${ARG_RESOURCE_PATHS})
    endforeach ()

    if (NOT ARG_SETTINGS_FILE)
        set(ARG_SETTINGS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/settings.json")
    endif ()

    if(NOT EXISTS ${ARG_SETTINGS_FILE})
        message(FATAL_ERROR "Failed to find settings file ${ARG_SETTINGS_FILE} for target ${TARGET_NAME}")
    endif ()

    set_target_properties(${TARGET_NAME} PROPERTIES PORTAL_SETTINGS_PATH ${ARG_SETTINGS_FILE})

    portal_read_settings(${TARGET_NAME})

    # TODO: add install + package targets
endfunction()