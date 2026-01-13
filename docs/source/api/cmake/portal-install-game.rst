.. _portal-install-game:

Function portal_install_game
----------------------------

Configures installation rules for a Portal Engine game executable with platform-specific
runtime dependency handling.

This function sets up comprehensive installation rules for game executables, including
the binary, runtime dependencies (DLLs/shared libraries), settings files, and resources.
It handles platform-specific RPATH configuration and excludes system libraries appropriately.

Synopsis
^^^^^^^^

.. code-block:: cmake

  portal_install_game(<target_name>)

Arguments
^^^^^^^^^

``<target_name>``
  Name of the game executable target to install.

Behavior
^^^^^^^^

The function performs the following operations:

1. **Executable Installation**: Installs the target binary and creates a runtime
   dependency set for automatic dependency resolution.

2. **Platform-Specific RPATH** (Linux/macOS)

3. **Runtime Dependency Installation**

4. **Settings File Installation**: If the target has a ``PORTAL_SETTINGS_PATH`` property,
   installs ``settings.json`` to the root installation directory.

5. **Resource Installation**: Installs all resources from both ``PORTAL_RESOURCES``
   (added via :ref:`portal_add_resources <portal-add-resources>`) and ``PORTAL_ADDITIONAL_RESOURCES``
   (added via :ref:`portal_fetch_resources <portal-fetch-resources>`) to ``resources/``.

Installation Layout
^^^^^^^^^^^^^^^^^^^

The installed game directory structure:

.. code-block:: text

  install/
    ├── <executable>          # Game binary (Windows/Linux)
    ├── <executable>.app/     # macOS bundle (if MAKE_STANDALONE)
    ├── settings.json         # Settings file (if present)
    ├── lib/                  # Shared libraries (Linux/macOS)
    │   ├── libvulkan.so.1
    │   └── ...
    └── resources/            # Game resources
        ├── textures/
        ├── models/
        └── ...

On Windows, DLLs are installed in the root alongside the executable.

Example Usage
^^^^^^^^^^^^^

Typically called internally by :ref:`portal_add_game <portal-add-game>`:

.. code-block:: cmake

  portal_install_game(my-game)

Install the game manually:

.. code-block:: bash

  cmake --install build/ninja-multi --component my-game

Notes
^^^^^

- This is an internal function typically called by :ref:`portal_add_game <portal-add-game>`
- Uses CMake's ``RUNTIME_DEPENDENCY_SET`` for automatic dependency discovery
- Resources from both ``PORTAL_RESOURCES`` and ``PORTAL_ADDITIONAL_RESOURCES`` are installed

See Also
^^^^^^^^

- :ref:`portal_add_game <portal-add-game>`: Creates Portal Engine game executables
- :ref:`portal_package_game <portal-package-game>`: Package game for distribution with CPack
- :ref:`portal_add_resources <portal-add-resources>`: Add local resources
- :ref:`portal_fetch_resources <portal-fetch-resources>`: Fetch resources from dependencies