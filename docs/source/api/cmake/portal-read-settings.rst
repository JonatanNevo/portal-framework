.. _portal-read-settings:

Function portal_read_settings
-----------------------------

Reads and processes a Portal Engine settings file, configuring the target based on its
contents and preparing it for installation.

This function reads a JSON settings file, extracts engine configuration and resource
references, sets up automatic resource copying, and configures the settings file as
an install target for deployment.

Synopsis
^^^^^^^^

.. code-block:: cmake

  portal_read_settings(<target_name>)

Arguments
^^^^^^^^^

``<target_name>``
  Name of the target to configure. The target must have the ``PORTAL_SETTINGS_PATH``
  property set (typically by :ref:`portal_add_game <portal-add-game>`).

Behavior
^^^^^^^^

The function performs the following operations:

1. **Settings Validation**: Retrieves the settings file path from the target's
   ``PORTAL_SETTINGS_PATH`` property and validates that the file exists.

2. **Installation Setup**: Creates a custom command to copy the settings file to
   ``$<TARGET_FILE_DIR:target>/<filename>`` using ``copy_if_different`` for
   efficient incremental builds.

3. **Display Name**: Reads ``project.name`` from the settings JSON to set the
   ``PORTAL_DISPLAY_NAME`` target property, defaulting to the target name on error.

4. **Configuration Parsing**: Reads and parses the JSON settings file to extract
   resource entries from ``project.resources``.

5. **Resource Configuration**: For each resource entry in the settings:

   - Extracts the ``type`` and ``path`` fields from the resource object
   - Validates that the path is relative (absolute paths are skipped)
   - Creates custom targets to copy resource directories to the runtime output
   - Registers resources with the ``PORTAL_RESOURCES`` property for installation

6. **Target Dependencies**: Ensures the settings file and resources are copied
   during the build process by adding appropriate target dependencies.


Example Usage
^^^^^^^^^^^^^

Typically called internally by :ref:`portal_add_game <portal-add-game>`:

.. code-block:: cmake

  # Set the settings path property
  set_target_properties(my-game PROPERTIES
      PORTAL_SETTINGS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/settings.json")

  # Read, configure, and prepare for installation
  portal_read_settings(my-game)

Notes
^^^^^

- This is an internal function typically called by :ref:`portal_add_game <portal-add-game>`
- Uses ``copy_if_different`` to avoid unnecessary rebuilds
- Only relative resource paths are processed
- Resources are tracked via ``PORTAL_RESOURCES`` for installation
- TODO: Future versions will validate engine version compatibility

See Also
^^^^^^^^

- :ref:`portal_add_game <portal-add-game>`: Creates Portal Engine game executables
- :ref:`portal_install_game <portal-install-game>`: Install game with settings and resources
- :ref:`portal_add_resources <portal-add-resources>`: Manually add resource directories
