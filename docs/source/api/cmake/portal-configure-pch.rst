.. _portal-configure-pch:

Function portal_configure_pch
-----------------------------

Configures precompiled headers for a target with optional CMake variable substitution.

This macro is typically used for setting up module-specific precompiled headers
that may need configuration-time processing (e.g., ``.inc`` files with CMake variables).

Synopsis
^^^^^^^^

.. code-block:: cmake

  portal_configure_pch(<target_name> <precompiled_headers>
                       [CONFIGURE])

Arguments
^^^^^^^^^

``<target_name>``
  Name of the target to add precompiled headers to. Can be any CMake target,
  not just Portal modules.

``<precompiled_headers>``
  List of header files to use as precompiled headers. Can be paths relative
  to ``CMAKE_CURRENT_SOURCE_DIR``.

``CONFIGURE``
  Optional flag indicating that headers should be processed through
  ``configure_file()`` before being used as PCH. Useful for headers containing
  CMake variable references (``@VAR@`` or ``${VAR}``).

Behavior
^^^^^^^^

The function performs the following operations for each header:

1. **Header Processing**:

   - If ``CONFIGURE`` is set:
     - Strips ``.inc`` suffix from the filename
     - Processes the header through ``configure_file()`` to substitute CMake variables
     - Outputs configured header to ``${CMAKE_CURRENT_BINARY_DIR}``
   - If ``CONFIGURE`` is not set:
     - Uses the header as-is from ``${CMAKE_CURRENT_SOURCE_DIR}``

2. **PCH Setup**: Adds the header (configured or original) as a PUBLIC
   precompiled header to the target.

3. **File Set Addition**: Adds the header to the target's ``additional_headers``
   file set, making it available for installation and export.

Example Usage
^^^^^^^^^^^^^

Using headers as-is (no configuration):

.. code-block:: cmake

  portal_configure_pch(portal-core
                       portal/core/pch.h)

Using headers with CMake variable substitution:

.. code-block:: cmake

  # portal/core/config.h.inc contains CMake variables like @PROJECT_VERSION@
  portal_configure_pch(portal-core
                       portal/core/config.h.inc
                       CONFIGURE)
  # Generates: ${CMAKE_CURRENT_BINARY_DIR}/portal/core/config.h

Multiple headers with configuration:

.. code-block:: cmake

  set(PCH_HEADERS
      portal/core/config.h.inc
      portal/core/platform.h.inc
  )
  portal_configure_pch(portal-core ${PCH_HEADERS} CONFIGURE)

Notes
^^^^^

- Configured headers have the ``.inc`` extension automatically removed
- All headers are added as PUBLIC PCH, propagating to dependent targets
- Headers are added to the ``additional_headers`` file set for proper installation

See Also
^^^^^^^^

- :ref:`portal_setup_config_pch <portal-setup-config-pch>`: Generates and configures config PCH automatically
- :ref:`portal_add_module <portal-add-module>`: Main module creation function