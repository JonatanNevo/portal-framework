.. _portal-add-module:

Function portal_add_module
--------------------------

Creates a Portal Framework module with full configuration including dependencies,
platform-specific sources, precompiled headers, and documentation.

This is the main entry point for creating Portal modules. It orchestrates all
aspects of module creation and configuration.

Synopsis
^^^^^^^^

.. code-block:: cmake

  portal_add_module(<module_name>
                    SOURCES <source>...
                    HEADERS <header>...
                    [WINDOWS_SOURCES <source>...]
                    [WINDOWS_HEADERS <header>...]
                    [MACOS_SOURCES <source>...]
                    [MACOS_HEADERS <header>...]
                    [LINUX_SOURCES <source>...]
                    [LINUX_HEADERS <header>...]
                    [COMPILE_CONFIG_FILE <file>]
                    [PORTAL_FIND_PACKAGE <bool>]
                    [PORTAL_DEPENDENCIES <dep>...]
                    [DEPENDENCIES <dep>...]
                    [COMPLEX_DEPENDENCIES <dep_spec>...])

Arguments
^^^^^^^^^

``<module_name>``
  Name of the module (without ``portal-`` prefix). The function creates a
  target named ``portal-<module_name>`` and an alias ``portal::<module_name>``.

``SOURCES <source>...``
  Required. List of source files (.cpp, .cc, etc.) for the module.

``HEADERS <header>...``
  Required. List of public header files to install and export.

``WINDOWS_SOURCES <source>...``
  Optional. Windows-specific source files (used only when building on Windows).

``WINDOWS_HEADERS <header>...``
  Optional. Windows-specific public headers (used only when building on Windows).

``MACOS_SOURCES <source>...``
  Optional. macOS-specific source files (used only when building on macOS).

``MACOS_HEADERS <header>...``
  Optional. macOS-specific public headers (used only when building on macOS).

``LINUX_SOURCES <source>...``
  Optional. Linux-specific source files (used only when building on Linux).

``LINUX_HEADERS <header>...``
  Optional. Linux-specific public headers (used only when building on Linux).

``COMPILE_CONFIG_FILE <file>``
  Optional. Path to the module's configuration header (e.g., ``portal/core/config.h``).
  This is included in the generated config PCH and exported for dependent modules.

``PORTAL_FIND_PACKAGE <bool>``
  Optional. Whether to call ``find_package()`` for Portal dependencies. Set to
  ``ON`` when consuming installed Portal modules, ``OFF`` for in-tree builds.

``PORTAL_DEPENDENCIES <dep>...``
  Optional. List of Portal module dependencies (names without ``portal-`` prefix).
  These are linked with ``portal::<dep>`` and their config headers are collected.

``DEPENDENCIES <dep>...``
  Optional. List of external library dependencies. Each is found via
  ``find_package()`` and linked with ``<dep>::<dep>``.

``COMPLEX_DEPENDENCIES <dep_spec>...``
  Optional. List of dependencies with custom options. Each spec is formatted as
  ``<dep_name>|<arg1>|<arg2>...`` where args are passed to :ref:`portal_add_dependency <portal-add-dependency>`.

Behavior
^^^^^^^^

The function performs the following operations:

1. **Target Creation**: Creates a STATIC library named ``portal-<module_name>``
   with C++23 standard and creates an alias ``portal::<module_name>``.

2. **File Sets**: Adds sources and headers with proper file sets for installation.

3. **Platform-Specific Sources**: Conditionally adds platform-specific sources
   and headers based on the build platform (Windows, macOS, or Linux).

4. **Compiler Flags**:

   - MSVC: ``/W4 /WX`` (warning level 4, warnings as errors)
   - Clang/GCC: ``-Wall -Wextra -Wpedantic -Werror -Wno-missing-designated-field-initializers``

5. **Dependency Resolution**: Processes all dependencies via :ref:`portal_add_dependency <portal-add-dependency>`:

   - External dependencies from ``DEPENDENCIES``
   - Portal dependencies from ``PORTAL_DEPENDENCIES``
   - Complex dependencies from ``COMPLEX_DEPENDENCIES``

6. **Config PCH Setup**: Calls :ref:`portal_setup_config_pch <portal-setup-config-pch>` to generate and
   configure precompiled headers with all config files from dependencies.

7. **Documentation**: If ``PORTAL_BUILD_DOCS`` is enabled, calls
   :ref:`portal_register_docs <portal-register-docs>` to set up Doxygen documentation generation.

Target Properties
^^^^^^^^^^^^^^^^^

The created target has the following properties:

- ``EXPORT_NAME``: Set to ``<module_name>`` (without ``portal-`` prefix)
- ``CXX_STANDARD``: 23
- ``CXX_STANDARD_REQUIRED``: ON
- ``CXX_EXTENSIONS``: OFF
- ``PORTAL_CONFIG_HEADERS``: List of configuration headers (exported)

Example Usage
^^^^^^^^^^^^^

Basic module with external dependencies:

.. code-block:: cmake

  portal_add_module(core
      SOURCES
          src/log.cpp
          src/memory.cpp
      HEADERS
          portal/core/log.h
          portal/core/memory.h
      COMPILE_CONFIG_FILE
          portal/core/config.h
      DEPENDENCIES
          spdlog
          glm
          mimalloc
  )

Module with Portal dependencies:

.. code-block:: cmake

  portal_add_module(engine
      SOURCES
          src/renderer.cpp
          src/scene.cpp
      HEADERS
          portal/engine/renderer.h
          portal/engine/scene.h
      COMPILE_CONFIG_FILE
          portal/engine/config.h
      PORTAL_DEPENDENCIES
          core
          application
          input
      DEPENDENCIES
          Vulkan
          imgui
  )

Module with platform-specific code:

.. code-block:: cmake

  portal_add_module(core
      SOURCES
          src/common.cpp
      HEADERS
          portal/core/common.h
      WINDOWS_SOURCES
          src/windows/file_system.cpp
      WINDOWS_HEADERS
          portal/platform/core/windows/file_system.h
      MACOS_SOURCES
          src/macos/file_system.mm
      MACOS_HEADERS
          portal/platform/core/macos/file_system.h
      LINUX_SOURCES
          src/linux/file_system.cpp
      LINUX_HEADERS
          portal/platform/core/linux/file_system.h
  )

Module with complex dependencies:

.. code-block:: cmake

  portal_add_module(networking
      SOURCES
          src/network.cpp
      HEADERS
          portal/networking/network.h
      PORTAL_DEPENDENCIES
          core
      COMPLEX_DEPENDENCIES
          "GameNetworkingSockets|LINK|GameNetworkingSockets::GameNetworkingSockets"
          "OpenSSL|FIND_PACKAGE_ARGS|REQUIRED|COMPONENTS|SSL|Crypto"
  )

Notes
^^^^^

- All modules are built as STATIC libraries
- All dependencies are linked as PUBLIC to propagate to consumers
- The module automatically participates in documentation generation when enabled
- Config headers from Portal dependencies are automatically collected and included
  in the generated config PCH

See Also
^^^^^^^^

- :ref:`portal_add_dependency <portal-add-dependency>`: Handles individual dependency configuration
- :ref:`portal_setup_config_pch <portal-setup-config-pch>`: Sets up configuration precompiled headers
- :ref:`portal_register_docs <portal-register-docs>`: Registers module for documentation generation
- :ref:`portal_install_module <portal-install-module>`: Installs the module with CMake package config