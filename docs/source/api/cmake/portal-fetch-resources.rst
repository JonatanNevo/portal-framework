.. _portal-fetch-resources:

Function portal_fetch_resources
-------------------------------

Copies resources from another Portal target (local or installed) to the current target's
runtime directory.

This function enables resource sharing between Portal targets by copying resources from
a dependency target (e.g., ``portal::engine``) to the current target. It works with both
local in-tree targets and installed/imported Portal modules.

Synopsis
^^^^^^^^

.. code-block:: cmake

  portal_fetch_resources(<target_name> <target_to_fetch>
                         [RESOURCES_TO_FETCH <resource>...])

Arguments
^^^^^^^^^

``<target_name>``
  Name of the target that will receive the fetched resources.

``<target_to_fetch>``
  Name of the Portal target to fetch resources from. Can be either a local
  target or an installed target (e.g., ``portal::engine``, ``portal::core``).

``RESOURCES_TO_FETCH <resource>...``
  Optional. List of specific resource folder names to fetch. If not specified,
  all resources from the target will be fetched. Each resource name must exist
  in the target's ``PORTAL_RESOURCES`` property or an error will be raised.

Behavior
^^^^^^^^

The function performs the following operations:

1. **Resource Validation**: Reads the ``PORTAL_RESOURCES`` property from
   ``<target_to_fetch>``. If specific resources are requested via ``RESOURCES_TO_FETCH``,
   validates that they exist.

2. **Path Resolution**: Determines the resource base path:

   - **Local targets**: Uses ``$<TARGET_FILE_DIR:target_to_fetch>/resources``
   - **Installed targets**: Uses the ``PORTAL_RESOURCE_PREFIX`` property set during
     installation

3. **Custom Target Creation**: For each resource, creates a custom target named
   ``<target_name>_fetch_<resource>_from_<target_to_fetch>`` that copies the resource
   directory to ``$<TARGET_FILE_DIR:target_name>/resources/<resource>/``.

4. **Dependency Chain**: Sets up dependencies to ensure correct build order:

   - Fetch target depends on ``<target_to_fetch>``
   - Fetch target depends on source copy target (if it exists)
   - Main target depends on fetch target

5. **Property Tracking**: Appends fetched resources to the target's
   ``PORTAL_ADDITIONAL_RESOURCES`` property and marks it for export.

Example Usage
^^^^^^^^^^^^^

Fetch all resources from portal-engine:

.. code-block:: cmake

  # Fetches all resources (shaders, fonts, icons, etc.)
  portal_fetch_resources(my-game portal::engine)

Fetch specific resources only:

.. code-block:: cmake

  # Only fetch shaders and fonts, ignore other engine resources
  portal_fetch_resources(my-game portal::engine
                         RESOURCES_TO_FETCH shaders fonts)

Fetch from multiple targets:

.. code-block:: cmake

  portal_fetch_resources(my-game portal::engine)
  portal_fetch_resources(my-game custom-asset-library RESOURCES_TO_FETCH models)

Notes
^^^^^

- Fetched resources are tracked separately via ``PORTAL_ADDITIONAL_RESOURCES``
- The ``PORTAL_RESOURCE_PREFIX`` property is set by :ref:`portal_install_module <portal-install-module>`
- Target namespace separators (``::``  ) are normalized to underscores for target names
- Errors if the requested resource doesn't exist in the source target

See Also
^^^^^^^^

- :ref:`portal_add_resources <portal-add-resources>`: Add local resources to a target
- :ref:`portal_install_module <portal-install-module>`: Install Portal modules (sets PORTAL_RESOURCE_PREFIX)
- :ref:`portal_read_settings <portal-read-settings>`: Read settings and process resource references
- :ref:`portal_install_game <portal-install-game>`: Install game with fetched resources