.. _portal-setup-config-pch:

Function portal_setup_config_pch
--------------------------------

Sets up configuration precompiled headers for a Portal module.

This function generates and configures a precompiled header that includes
all configuration headers from the module and its Portal dependencies.

Synopsis
^^^^^^^^

.. code-block:: cmake

  portal_setup_config_pch(<module_name>
                          [GAME]
                          [COMPILE_CONFIG_FILE <file>]
                          [DEPENDENT_CONFIG_HEADERS <header>...])

Arguments
^^^^^^^^^

``<module_name>``
  Name of the module (without ``portal-`` prefix) to set up config PCH for.

``GAME``
  Optional flag indicating this is a game target rather than a Portal module.
  Changes the target name from ``portal-<module_name>`` to just ``<module_name>``
  and adjusts the PCH output path accordingly.

``COMPILE_CONFIG_FILE <file>``
  Optional. Path to the module's own configuration header file. This will be
  added to the ``PORTAL_CONFIG_HEADERS`` property and included in the generated PCH.

``DEPENDENT_CONFIG_HEADERS <header>...``
  Optional. List of configuration headers from Portal dependencies. These are
  typically collected from ``PORTAL_CONFIG_HEADERS`` properties of dependent modules.

Behavior
^^^^^^^^

The function performs the following operations:

1. **Property Setup**: Sets the ``PORTAL_CONFIG_HEADERS`` target property with
   all config headers (both dependent and module-specific) and marks it for export.

2. **PCH Generation**: Creates an auto-generated precompiled header file at:

   - For Portal modules: ``${CMAKE_CURRENT_BINARY_DIR}/portal/<module_name>/config_pch.h``
   - For game targets (``GAME`` flag): ``${CMAKE_CURRENT_BINARY_DIR}/<module_name>/config_pch.h``

3. **PCH Content**: The generated header includes:

   - ``#pragma once`` guard
   - ``#include`` directives for all configuration headers
   - Auto-generation comment

4. **Target Configuration**: Adds the generated header as a PUBLIC precompiled
   header to the target, making it available to all dependent targets.

Example Usage
^^^^^^^^^^^^^

Setting up config PCH with module's own config file:

.. code-block:: cmake

  portal_setup_config_pch(core
                          COMPILE_CONFIG_FILE portal/core/config.h)

Setting up config PCH with dependent configuration headers:

.. code-block:: cmake

  portal_setup_config_pch(engine
                          COMPILE_CONFIG_FILE portal/engine/config.h
                          DEPENDENT_CONFIG_HEADERS
                              portal/core/config.h
                              portal/application/config.h)

Setting up for a game target:

.. code-block:: cmake

  portal_setup_config_pch(my_game
                          GAME
                          DEPENDENT_CONFIG_HEADERS portal/engine/config.h)

Notes
^^^^^

- The ``PORTAL_CONFIG_HEADERS`` property is exported to allow consumers to access
  configuration headers when the module is imported
- Config headers from dependencies are automatically collected by :ref:`portal_add_module <portal-add-module>`
- The generated PCH is PUBLIC, propagating to all dependent targets
- This function is automatically called by :ref:`portal_add_module <portal-add-module>`

See Also
^^^^^^^^

- :ref:`portal_add_module <portal-add-module>`: Main module creation function that calls this
- :ref:`portal_configure_pch <portal-configure-pch>`: Alternative PCH configuration for non-config headers