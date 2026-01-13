.. _portal-package-game:

Function portal_package_game
----------------------------

Configures CPack packaging for a Portal Engine game executable.

This function sets up CPack configuration to create distributable packages
(installers, archives, etc.) for the game. It registers the game as a CPack
component with metadata for package generation.

Synopsis
^^^^^^^^

.. code-block:: cmake

  portal_package_game(<target_name> <display_name>
                      [VENDOR <vendor>]
                      [CONTACT <contact>])

Arguments
^^^^^^^^^

``<target_name>``
  Name of the game executable target to package.

``<display_name>``
  Human-readable name for the package (e.g., "My Awesome Game").

``VENDOR <vendor>``
  Optional. Vendor/publisher name for the package (e.g., "Some Games Company Inc.").

``CONTACT <contact>``
  Optional. Contact information for the package maintainer.

Behavior
^^^^^^^^

The function performs the following operations:

1. **CPack Inclusion**: Includes the CPack module to enable packaging functionality.

2. **Package Configuration**: Sets CPack variables:

   - ``CPACK_PACKAGE_NAME``: Set to ``<display_name>``
   - ``CPACK_PACKAGE_VENDOR``: Set to ``<vendor>`` if provided
   - ``CPACK_PACKAGE_INSTALL_DIRECTORY``: Set to ``<display_name>``
   - ``CPACK_VERBATIM_VARIABLES``: Enabled for consistent behavior
   - ``CPACK_COMPONENTS_GROUPING``: Set to ``IGNORE`` for flat component structure

3. **Component Registration**: Adds the target as a required CPack component
   with the project name as its display name.

Example Usage
^^^^^^^^^^^^^

Typically called internally by ``portal_add_game()``:

.. code-block:: cmake

  portal_package_game(my-game "My Awesome Game"
                      VENDOR "Some Games Company Inc."
                      CONTACT "support@some_game_company.com")

Notes
^^^^^

- This is an internal function typically called by ``portal_add_game()``
- Requires ``portal_install_game`` to be called first for proper installation rules
- The actual package format depends on the platform and available generators

See Also
^^^^^^^^

- :ref:`portal_add_game <portal-add-game>`: Creates Portal Engine game executables
- :ref:`portal_install_game <portal-install-game>`: Sets up installation rules for the game
