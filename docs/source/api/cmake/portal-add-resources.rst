.. _portal-add-resources:

Function portal_add_resources
-----------------------------

Adds a resource directory to a target, automatically copying it to the runtime output directory.

This function sets up automatic copying of resource directories (e.g., textures, models, shaders)
to the target's runtime directory structure. Resources are tracked via the ``PORTAL_RESOURCES``
target property, which is used during installation and by ``portal_fetch_resources``.

Synopsis
^^^^^^^^

.. code-block:: cmake

  portal_add_resources(<target_name> <resource_path>
                       [OUTPUT_NAME <name>])

Arguments
^^^^^^^^^

``<target_name>``
  Name of the target to add resources to.

``<resource_path>``
  Path to the resource directory, relative to ``CMAKE_CURRENT_SOURCE_DIR``.
  All files within this directory will be recursively copied.

``OUTPUT_NAME <name>``
  Optional. The name of the subdirectory under ``resources/`` in the output directory.
  If not specified, uses the base name of ``<resource_path>``.

Behavior
^^^^^^^^

The function performs the following operations:

1. **Resource Discovery**: Recursively finds all files within ``<resource_path>``
   using ``GLOB_RECURSE``.

2. **Custom Target Creation**: Creates a custom target named
   ``<target_name>_copy_resources_<output_name>`` that copies the resource directory to
   ``$<TARGET_FILE_DIR:target>/resources/<output_name>/`` at build time.

3. **Dependency Setup**: Adds the custom target as a dependency of the main target,
   ensuring resources are copied before the executable runs.

4. **Property Tracking**: Appends ``<output_name>`` to the target's ``PORTAL_RESOURCES``
   property and marks it as an exported property for installation.

Example Usage
^^^^^^^^^^^^^

Basic resource addition with automatic output name:

.. code-block:: cmake

  # Copies textures/ to build/resources/textures/
  portal_add_resources(my-game textures)

Resource addition with custom output name:

.. code-block:: cmake

  # Copies game_data/maps/ to build/resources/game-maps/
  portal_add_resources(my-game game_data/maps OUTPUT_NAME game-maps)

Multiple resources for one target:

.. code-block:: cmake

  portal_add_resources(my-game textures)
  portal_add_resources(my-game models)
  portal_add_resources(my-game shaders)
  # Results in: resources/textures/, resources/models/, resources/shaders/

Notes
^^^^^

- Resources are copied to ``$<TARGET_FILE_DIR:target>/resources/<output_name>/``
- The ``PORTAL_RESOURCES`` property is exported for use during installation
- Resources are automatically copied when the target is built
- Use :ref:`portal_fetch_resources <portal-fetch-resources>` to copy resources from other Portal targets

See Also
^^^^^^^^

- :ref:`portal_fetch_resources <portal-fetch-resources>`: Fetch resources from other Portal targets
- :ref:`portal_install_game <portal-install-game>`: Install game with resources
- :ref:`portal_read_settings <portal-read-settings>`: Read settings file and process resource references
