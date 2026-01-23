.. _portal-install-module:

Function portal_install_module
------------------------------

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

See Also
^^^^^^^^

- :ref:`portal_add_module <portal-add-module>`: Creates Portal modules
- :ref:`portal_add_resources <portal-add-resources>`: Add resources to modules
