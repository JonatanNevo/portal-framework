function(portal_setup_compile_configs TARGET_NAME APPLICATION_NAME SETTINGS_FILE ICON_FILE)
    set(CONFIGURE_FILE "
// Auto generated file

namespace portal
{
    constexpr std::string_view PORTAL_APPLICATION_NAME = \"@APPLICATION_NAME@\";
    constexpr std::string_view PORTAL_SETTINGS_FILE_NAME = \"@SETTINGS_FILE@\";
    constexpr std::string_view PORTAL_ICON_FILE_NAME =  \"@ICON_FILE@\";
};
"
    )

    set(INPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/config_impl.inc)
    set(OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/config_impl.cpp)
    file(WRITE "${INPUT_PATH}" "${CONFIGURE_FILE}")

    configure_file(${INPUT_PATH} ${OUTPUT_PATH} @ONLY)
    target_sources(${TARGET_NAME} PRIVATE "${OUTPUT_PATH}")
endfunction()

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
    if (NOT EXISTING_RESOURCES)
        set(EXISTING_RESOURCES "")
    endif ()

    list(APPEND EXISTING_RESOURCES "${ARG_OUTPUT_NAME}")
    set_target_properties(${TARGET_NAME} PROPERTIES PORTAL_RESOURCES "${EXISTING_RESOURCES}")
    set_property(TARGET ${TARGET_NAME} APPEND PROPERTY EXPORT_PROPERTIES PORTAL_RESOURCES)
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
    if (NOT EXISTING_ADDITIONAL_RESOURCES)
        set(EXISTING_ADDITIONAL_RESOURCES "")
    endif ()

    set(RESOURCES_TO_COPY ${TARGET_RESOURCES})
    if (ARG_RESOURCES_TO_FETCH)
        # Validate and use only specific resources
        set(RESOURCES_TO_COPY "")
        foreach (RESOURCE_PATH ${ARG_RESOURCES_TO_FETCH})
            if (NOT RESOURCE_PATH IN_LIST TARGET_RESOURCES)
                message(FATAL_ERROR "Resource '${RESOURCE_PATH}' not found in target ${TARGET_TO_FETCH}. Available resources: ${TARGET_RESOURCES}")
            endif ()
            list(APPEND RESOURCES_TO_COPY "${RESOURCE_PATH}")
        endforeach ()
    endif ()

    # Check if the target has a PORTAL_RESOURCE_PREFIX property (for installed targets)
    get_target_property(RESOURCE_PREFIX ${TARGET_TO_FETCH} PORTAL_RESOURCE_PREFIX)
    if (NOT RESOURCE_PREFIX OR RESOURCE_PREFIX STREQUAL "PORTAL_RESOURCE_PREFIX-NOTFOUND")
        # For local targets, use TARGET_FILE_DIR
        set(RESOURCE_BASE_PATH "$<TARGET_FILE_DIR:${TARGET_TO_FETCH}>/resources")
    else()
        # For installed/imported targets, use the PORTAL_RESOURCE_PREFIX property
        set(RESOURCE_BASE_PATH "${RESOURCE_PREFIX}")
    endif()

    foreach (RESOURCE_PATH ${RESOURCES_TO_COPY})
        list(APPEND EXISTING_ADDITIONAL_RESOURCES "${RESOURCE_PATH}")

        set(_TARGET_TO_FETCH_NORMALIZED "${TARGET_TO_FETCH}")
        string(REPLACE "::" "_" _TARGET_TO_FETCH_NORMALIZED "${_TARGET_TO_FETCH_NORMALIZED}")

        set(FETCH_TARGET_NAME "${TARGET_NAME}_fetch_${RESOURCE_PATH}_from_${_TARGET_TO_FETCH_NORMALIZED}")

        set(SOURCE_COPY_TARGET "${TARGET_TO_FETCH}_copy_${RESOURCE_PATH}")

        add_custom_target(${FETCH_TARGET_NAME}
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${RESOURCE_BASE_PATH}/${RESOURCE_PATH}"
                "$<TARGET_FILE_DIR:${TARGET_NAME}>/resources/${RESOURCE_PATH}"
                COMMENT "Fetching resources for ${TARGET_NAME}: ${TARGET_TO_FETCH}/resources/${RESOURCE_PATH} -> resources/${RESOURCE_PATH}"
                VERBATIM
        )

        if (TARGET ${SOURCE_COPY_TARGET})
            add_dependencies(${FETCH_TARGET_NAME} ${SOURCE_COPY_TARGET})
        endif ()
        add_dependencies(${FETCH_TARGET_NAME} ${TARGET_TO_FETCH})

        add_dependencies(${TARGET_NAME} ${FETCH_TARGET_NAME})
    endforeach ()

    set_target_properties(${TARGET_NAME} PROPERTIES PORTAL_ADDITIONAL_RESOURCES "${EXISTING_ADDITIONAL_RESOURCES}")
    set_property(TARGET ${TARGET_NAME} APPEND PROPERTY EXPORT_PROPERTIES PORTAL_ADDITIONAL_RESOURCES)
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
    if (RESOURCES_LIST_LENGTH GREATER 0)
        math(EXPR RESOURCES_LIST_LAST_INDEX "${RESOURCES_LIST_LENGTH} - 1")
        foreach (IDX RANGE ${RESOURCES_LIST_LAST_INDEX})
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
                set_property(TARGET ${TARGET_NAME} APPEND PROPERTY PORTAL_RESOURCES ${RESOURCE_PATH})
            else ()
                message(STATUS "Found absolute resource path, skipping... ${RESOURCE_PATH}")
            endif ()
        endforeach ()
    endif ()
endfunction()

function(portal_install_game TARGET_NAME)
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    install(
            TARGETS ${TARGET_NAME}
            COMPONENT ${TARGET_NAME}
            RUNTIME_DEPENDENCY_SET ${TARGET_NAME}_deps
            RUNTIME
                DESTINATION .
            BUNDLE
                DESTINATION .
    )

    message(STATUS "Vcpkg Installed Dir ${VCPKG_INSTALLED_DIR}")

    if (WIN32)
        install(
                RUNTIME_DEPENDENCY_SET ${TARGET_NAME}_deps
                PRE_EXCLUDE_REGEXES
                # Windows API Set DLLs
                "api-ms-win-.*" "ext-ms-.*"
                POST_EXCLUDE_REGEXES
                # Windows system directories
                ".*[Ss]ystem32/.*\\.dll" ".*[Ww]in[Ss]x[Ss]/.*\\.dll"
                DIRECTORIES
                $<TARGET_FILE_DIR:${TARGET_NAME}>
                ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib
                ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin
                COMPONENT ${TARGET_NAME}
                DESTINATION .
        )
    else ()
        if (APPLE)
            set_target_properties(${TARGET_NAME} PROPERTIES
                    BUILD_RPATH "@executable_path/${TARGET_NAME}.app/Contents/Frameworks"
                    BUILD_WITH_INSTALL_RPATH FALSE
            )
        elseif (UNIX)
            set_target_properties(${TARGET_NAME} PROPERTIES
                    INSTALL_RPATH "$ORIGIN/lib"
                    BUILD_RPATH "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib"
                    BUILD_WITH_INSTALL_RPATH FALSE
            )
        endif ()

        install(
                RUNTIME_DEPENDENCY_SET ${TARGET_NAME}_deps
                PRE_EXCLUDE_REGEXES
                # Common system libraries (all platforms)
                "libc\\.so.*" "libm\\.so.*" "libpthread\\.so.*" "libdl\\.so.*"
                "librt\\.so.*" "libgcc_s\\.so.*" "libstdc\\+\\+\\.so.*"
                "ld-linux.*\\.so.*" "libSystem\\..*dylib" "libc\\+\\+\\..*dylib"
                POST_EXCLUDE_REGEXES
                # Linux system directories
                "^/lib/.*\\.so.*" "^/lib64/.*\\.so.*"
                ".*/usr/lib/.*\\.so.*" ".*/usr/lib64/.*\\.so.*"
                # macOS system directories
                ".*/System/Library/.*" ".*/usr/lib/.*\\.dylib"
                DIRECTORIES
                $<TARGET_FILE_DIR:${TARGET_NAME}>
                ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib
                ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin
                COMPONENT ${TARGET_NAME}
                DESTINATION lib
        )
    endif ()

    set(FILES_TO_INSTALL "")

    get_target_property(SETTINGS_PATH ${TARGET_NAME} PORTAL_SETTINGS_PATH)
    if (SETTINGS_PATH)
        list(APPEND FILES_TO_INSTALL "${CMAKE_CURRENT_SOURCE_DIR}/settings.json")
    endif ()

    if (FILES_TO_INSTALL)
        install(
                FILES ${FILES_TO_INSTALL}
                COMPONENT ${TARGET_NAME}
                DESTINATION .
        )
    endif ()


    get_target_property(RESOURCES_FOLDERS ${TARGET_NAME} PORTAL_RESOURCES)
    if (NOT RESOURCES_FOLDERS OR RESOURCES_FOLDERS STREQUAL "PORTAL_RESOURCES-NOTFOUND")
        set(RESOURCES_FOLDERS "")
    endif ()

    foreach (RESOURCE_FOLDER IN LISTS RESOURCES_FOLDERS)
        install(
                DIRECTORY "$<TARGET_FILE_DIR:${TARGET_NAME}>/resources/${RESOURCE_FOLDER}"
                COMPONENT ${TARGET_NAME}
                DESTINATION resources
        )
    endforeach ()

    get_target_property(ADDITIONAL_RESOURCES_FOLDERS ${TARGET_NAME} PORTAL_ADDITIONAL_RESOURCES)
    if (NOT ADDITIONAL_RESOURCES_FOLDERS OR ADDITIONAL_RESOURCES_FOLDERS STREQUAL "PORTAL_ADDITIONAL_RESOURCES-NOTFOUND")
        set(ADDITIONAL_RESOURCES_FOLDERS "")
    endif ()

    foreach (RESOURCE_FOLDER IN LISTS ADDITIONAL_RESOURCES_FOLDERS)
        install(
                DIRECTORY "$<TARGET_FILE_DIR:${TARGET_NAME}>/resources/${RESOURCE_FOLDER}"
                COMPONENT ${TARGET_NAME}
                DESTINATION resources
        )
    endforeach ()
endfunction()

function(portal_package_game TARGET_NAME DISPLAY_NAME)
    set(options "")
    set(oneValueArgs
            VENDOR
            CONTACT
    )
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})


    include(CPack)
    set(CPACK_PACKAGE_NAME ${DISPLAY_NAME})
    set(CPACK_PACKAGE_VENDOR ${ARG_VENDOR})
    set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
    set(CPACK_VERBATIM_VARIABLES YES)

    set(CPACK_COMPONENTS_GROUPING IGNORE)

    cpack_add_component(${TARGET_NAME}
            REQUIRED
            DISPLAY_NAME ${CMAKE_PROJECT_NAME}
    )
endfunction()

macro(_find_icon_files STATIC_ICON LOGO_FILE)
    # Get the resource prefix from portal-engine for default icons
    get_target_property(ENGINE_RESOURCE_PREFIX portal::engine PORTAL_RESOURCE_PREFIX)
    if (NOT ENGINE_RESOURCE_PREFIX OR ENGINE_RESOURCE_PREFIX STREQUAL "ENGINE_RESOURCE_PREFIX-NOTFOUND")
        # Fallback for local builds (not installed)
        set(ENGINE_RESOURCE_PREFIX "${CMAKE_SOURCE_DIR}/engine/resources")
    else()
        # For installed targets, append the engine folder
        set(ENGINE_RESOURCE_PREFIX "${ENGINE_RESOURCE_PREFIX}/engine")
    endif()

    if (STATIC_ICON)
        cmake_path(REPLACE_EXTENSION STATIC_ICON "icns" OUTPUT_VARIABLE ICNS_FILE_CHECK)
        cmake_path(REPLACE_EXTENSION STATIC_ICON "ico" OUTPUT_VARIABLE ICO_FILE_CHECK)
        cmake_path(REPLACE_EXTENSION STATIC_ICON "png" OUTPUT_VARIABLE PNG_FILE_CHECK)
    else()
        set(ICNS_FILE_CHECK "")
        set(ICO_FILE_CHECK "")
        set(PNG_FILE_CHECK "")
    endif()

    if (NOT LOGO_FILE)
        set(LOGO_FILE "")
    endif ()

    if (EXISTS "${ICNS_FILE_CHECK}")
        set(ICON_ICNS_FILE "${ICNS_FILE_CHECK}")
        message(STATUS "Using MacOS Icon: ${ICON_ICNS_FILE}")
    else()
        set(ICON_ICNS_FILE "${ENGINE_RESOURCE_PREFIX}/portal_icon_64x64.icns")
        message(STATUS "Missing MacOS Icon, defaulting to: ${ICON_ICNS_FILE}")
    endif()

    if (EXISTS "${ICO_FILE_CHECK}")
        set(ICON_ICO_FILE "${ICO_FILE_CHECK}")
        message(STATUS "Using Windows Icon: ${ICON_ICO_FILE}")
    else()
        set(ICON_ICO_FILE "${ENGINE_RESOURCE_PREFIX}/portal_icon_64x64.ico")
        message(STATUS "Missing Windows Icon, defaulting to: ${ICON_ICO_FILE}")
    endif()

    if (EXISTS "${PNG_FILE_CHECK}")
        set(ICON_PNG_FILE "${PNG_FILE_CHECK}")
        message(STATUS "Using png Icon: ${ICON_PNG_FILE}")
    else()
        set(ICON_PNG_FILE "${ENGINE_RESOURCE_PREFIX}/portal_icon_64x64.png")
        message(STATUS "Missing png Icon, defaulting to: ${ICON_PNG_FILE}")
    endif()

    if (EXISTS "${LOGO_FILE}")
        set(LOGO_PNG_FILE "${LOGO_FILE}")
        message(STATUS "Using Logo: ${LOGO_FILE}")
    else()
        set(LOGO_PNG_FILE "${ENGINE_RESOURCE_PREFIX}/portal_logo_dark.png")
        message(STATUS "Missing Logo, defaulting to: ${LOGO_PNG_FILE}")
    endif()
endmacro()

function(portal_add_game TARGET_NAME)
    set(options MAKE_STANDALONE)
    set(oneValueArgs
            SETTINGS_FILE
            STATIC_ICON
            LOGO_FILE
            DISPLAY_NAME
            SETTINGS_FILE_NAME
            VENDOR
            CONTACT
    )
    set(multiValueArgs
            SOURCES
            RESOURCE_PATHS
            LINK_LIBRARIES
    )
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_DISPLAY_NAME)
        set(ARG_DISPLAY_NAME ${TARGET_NAME})
    endif ()

    if (NOT ARG_SETTINGS_FILE_NAME)
        set(ARG_SETTINGS_FILE_NAME "settings.json")
    endif ()

    if (NOT ARG_STATIC_ICON)
        set(ARG_STATIC_ICON "")
    endif ()

    if (NOT ARG_LOGO_FILE)
        set(ARG_LOGO_FILE "")
    endif ()

    _find_icon_files(ARG_STATIC_ICON ARG_LOGO_FILE)

    # TODO: add editor target
    if (APPLE AND ARG_MAKE_STANDALONE)
        cmake_path(GET ICON_ICNS_FILE FILENAME BUNDLE_ICON_FILE)
        set_source_files_properties(${ARG_STATIC_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

        add_executable(${TARGET_NAME} MACOSX_BUNDLE ${ARG_SOURCES} ${ARG_STATIC_ICON})
        set_target_properties(${TARGET_NAME} PROPERTIES
                MACOSX_BUNDLE TRUE
                MACOSX_BUNDLE_IDENTIFIER com.portal.${TARGET_NAME}
                MACOSX_BUNDLE_BUNDLE_NAME ${ARG_DISPLAY_NAME}
                MACOSX_BUNDLE_ICON_FILE ${BUNDLE_ICON_FILE}
                MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
                MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        )
    elseif (WIN32)
        if (ARG_MAKE_STANDALONE)
            add_executable(${TARGET_NAME} WIN32 ${ARG_SOURCES})
        else()
            add_executable(${TARGET_NAME} ${ARG_SOURCES})
        endif()
    else ()
        add_executable(${TARGET_NAME} ${ARG_SOURCES})
    endif ()

    set_target_properties(${TARGET_NAME} PROPERTIES PORTAL_WINDOWS_ICON ${ICON_ICO_FILE})
    set_target_properties(${TARGET_NAME} PROPERTIES PORTAL_MACOS_ICON ${ICON_ICNS_FILE})
    set_target_properties(${TARGET_NAME} PROPERTIES PORTAL_LOGO ${LOGO_PNG_FILE})

    if (ARG_MAKE_STANDALONE)
        target_compile_definitions(${TARGET_NAME} PRIVATE PORTAL_STANDALONE_EXE)
    endif ()

    target_link_libraries(
            ${TARGET_NAME}
            portal::engine
            ${ARG_LINK_LIBRARIES}
    )

    portal_fetch_resources(${TARGET_NAME} portal::engine)

    foreach (RESOURCE_PATH ${ARG_RESOURCE_PATHS})
        portal_add_resources(${TARGET_NAME} ${ARG_RESOURCE_PATHS})
    endforeach ()

    if (NOT ARG_SETTINGS_FILE)
        set(ARG_SETTINGS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_SETTINGS_FILE_NAME}")
    endif ()

    if (NOT EXISTS ${ARG_SETTINGS_FILE})
        message(FATAL_ERROR "Failed to find settings file ${ARG_SETTINGS_FILE} for target ${TARGET_NAME}")
    endif ()

    set_target_properties(${TARGET_NAME} PROPERTIES PORTAL_SETTINGS_PATH ${ARG_SETTINGS_FILE})

    portal_setup_compile_configs(${TARGET_NAME} ${ARG_DISPLAY_NAME} ${ARG_SETTINGS_FILE_NAME} ${ICON_PNG_FILE})

    portal_read_settings(${TARGET_NAME})

    portal_install_game(${TARGET_NAME})

    portal_package_game(
            ${TARGET_NAME}
            ${ARG_DISPLAY_NAME}
            VENDOR ${ARG_VENDOR}
            CONTACT ${ARG_CONTACT}
    )
endfunction()