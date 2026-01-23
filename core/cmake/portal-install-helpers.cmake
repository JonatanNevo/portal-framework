#[=======================================================================[.rst:
portal_install_module
---------------------

Installs a Portal Framework module with CMake package configuration support.

This function automates the installation process for Portal modules, including:
- Installing target exports with namespace ``portal::``
- Generating and installing CMake package config and version files
- Installing file sets (HEADERS and optional additional_headers)
- Installing associated resources
- Setting config/resource prefix metadata for installed modules
- Installing helper CMake scripts

Synopsis
^^^^^^^^

.. code-block:: cmake

  portal_install_module(<module_name>
                        [COMPONENT <component>]
                        [FILES <file>...])

Arguments
^^^^^^^^^

``<module_name>``
  Name of the module (without the ``portal-`` prefix). The function will
  automatically construct the target name as ``portal-<module_name>``.

``COMPONENT <component>``
  Optional. Specifies the installation component name. If not provided,
  defaults to the target name with hyphens replaced by underscores for
  NSIS compatibility.

``FILES <file>...``
  Optional. List of additional CMake helper files to install alongside
  the package config file. These files will be automatically included
  in the generated config file and installed to ``share/portal-<module_name>/``.

Behavior
^^^^^^^^

The function performs the following operations:

1. **Target Installation**: Installs the ``portal-<module_name>`` target with
   its HEADERS file set. If the target has an ``additional_headers`` file set,
   it will also be installed.

2. **Package Config Files**: Generates and installs:

   - ``portal-<module_name>-config.cmake``: Package configuration file
   - ``portal-<module_name>-version.cmake``: Version file with ExactVersion compatibility
   - ``portal-<module_name>-targets.cmake``: Target export file with ``portal::`` namespace

3. **Resources**: If the target has a ``PORTAL_RESOURCES`` property set, the
   function installs those resource directories and configures the
   ``PORTAL_RESOURCE_PREFIX`` property in the config file to allow consumers
   to locate resources correctly.

4. **Config Prefix**: If the target has ``PORTAL_HAS_CONFIG`` set, the generated
   config file sets ``PORTAL_CONFIG_PREFIX`` on the imported target so consumers
   can locate installed configs.

5. **Helper Files**: Any files specified via the ``FILES`` argument are installed
   and automatically included in the generated config file.

Installation Locations
^^^^^^^^^^^^^^^^^^^^^^

- Headers: ``${CMAKE_INSTALL_INCLUDEDIR}``
- CMake files: ``share/portal-<module_name>/``
- Resources: ``resources/``

Example Usage
^^^^^^^^^^^^^

Basic module installation:

.. code-block:: cmake

  portal_install_module(core)

Module with custom component and helper files:

.. code-block:: cmake

  portal_install_module(engine
                        COMPONENT engine_runtime
                        FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/engine-helpers.cmake)

Notes
^^^^^

- Uses experimental CMake package dependencies export feature
  (``CMAKE_EXPERIMENTAL_EXPORT_PACKAGE_DEPENDENCIES``)
- Resource folders are copied from the target's runtime output directory
- The generated config file uses ``@PACKAGE_INIT@`` for relocatable packages

#]=======================================================================]
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

    # Set the resource prefix for installed targets
    # This allows consumers to locate resources correctly when the target is imported
    get_target_property(RESOURCES_FOLDERS ${TARGET_NAME} PORTAL_RESOURCES)
    if (RESOURCES_FOLDERS AND NOT RESOURCES_FOLDERS STREQUAL "PORTAL_RESOURCES-NOTFOUND")
        string(APPEND CONFIG_CONTENT "\n# Set resource prefix for portal::${MODULE_NAME}\n")
        string(APPEND CONFIG_CONTENT "set_target_properties(portal::${MODULE_NAME} PROPERTIES\n")
        string(APPEND CONFIG_CONTENT "    PORTAL_RESOURCE_PREFIX \"\${PACKAGE_PREFIX_DIR}/resources\"\n")
        string(APPEND CONFIG_CONTENT ")\n")
    endif ()

    get_target_property(HAS_CONFIG ${TARGET_NAME} PORTAL_HAS_CONFIG)
    if (HAS_CONFIG AND NOT HAS_CONFIG STREQUAL "PORTAL_HAS_CONFIG-NOTFOUND")
        string(APPEND CONFIG_CONTENT "\n# Set config prefix for portal::${MODULE_NAME}\n")
        string(APPEND CONFIG_CONTENT "set_target_properties(portal::${MODULE_NAME} PROPERTIES\n")
        string(APPEND CONFIG_CONTENT "    PORTAL_CONFIG_PREFIX \"\${PACKAGE_PREFIX_DIR}/config\"\n")
        string(APPEND CONFIG_CONTENT ")\n")
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
