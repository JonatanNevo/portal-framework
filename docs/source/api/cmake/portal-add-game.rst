.. _portal-add-game:

Function portal_add_game
------------------------

Creates a complete Portal Engine game executable plus a companion editor target with
automatic resource management, settings configuration, installation, and packaging
for the runtime target.

This is the main entry point for creating game applications with the Portal Engine.
It orchestrates runtime and editor executable creation, icon/logo resolution, resource
fetching, settings processing, and automatic installation/packaging setup for the
runtime target.

Synopsis
^^^^^^^^

.. code-block:: cmake

  portal_add_game(<target_name>
                  SOURCES <source>...
                  [MAKE_STANDALONE]
                  [NO_CONFIG]
                  [SETTINGS_FILE <path>]
                  [SETTINGS_FILE_NAME <filename>]
                  [STATIC_ICON <path>]
                  [LOGO_FILE <path>]
                  [RESOURCE_PATHS <path>...]
                  [LINK_LIBRARIES <library>...]
                  [VENDOR <vendor>]
                  [CONTACT <contact>])

Arguments
^^^^^^^^^

``<target_name>``
  Name of the game executable target to create.

``SOURCES <source>...``
  Required. List of source files for the game executable.

``MAKE_STANDALONE``
  Optional flag. Creates a standalone application:

  - **Windows**: Creates a WIN32 subsystem executable (no console window)
  - **macOS**: Creates a MACOSX_BUNDLE with proper bundle structure
  - **Linux**: No effect (standard executable)

``NO_CONFIG``
  Optional flag. Skips adding the local ``config/`` directory via
  :ref:`portal_add_configs <portal-add-configs>`. Configs from dependencies
  (e.g., ``portal::engine``) are still fetched.

``SETTINGS_FILE <path>``
  Optional. Path to the settings JSON file. If not specified, defaults to
  ``${CMAKE_CURRENT_SOURCE_DIR}/<settings_file_name>``.

``SETTINGS_FILE_NAME <filename>``
  Optional. Name of the settings file. Defaults to ``settings.json``.

``STATIC_ICON <path>``
  Optional. Base path for platform-specific icon files (without extension).
  The function will look for:

  - ``.icns`` (macOS)
  - ``.ico`` (Windows)
  - ``.png`` (Linux/fallback)

  If not found, defaults to Portal Engine icons.

``LOGO_FILE <path>``
  Optional. Path to the logo PNG file. Defaults to Portal Engine logo if not provided.

``RESOURCE_PATHS <path>...``
  Optional. List of local resource directories to add via ``portal_add_resources``.

``LINK_LIBRARIES <library>...``
  Optional. Additional libraries to link beyond ``portal::engine``.

``VENDOR <vendor>``
  Optional. Vendor/publisher name for CPack packaging.

``CONTACT <contact>``
  Optional. Contact information for CPack packaging.

Behavior
^^^^^^^^

The function performs the following operations in order:

1. **Argument Defaults**: Sets default values for ``SETTINGS_FILE_NAME``
   ("settings.json") and initializes icon/logo inputs when not provided.

2. **Executable Creation**:

   - **Runtime target**: Uses MACOSX_BUNDLE or WIN32 when ``MAKE_STANDALONE`` is set,
     otherwise creates a standard console executable
   - **Editor target**: Always creates ``<target_name>_editor`` as a standard console
     executable using the same sources

3. **Icon Resolution**: Calls ``_find_icon_files`` to locate platform-specific
   icons or use Portal Engine defaults, then sets target properties on both targets:

   - ``PORTAL_WINDOWS_ICON``: Path to .ico file
   - ``PORTAL_MACOS_ICON``: Path to .icns file
   - ``PORTAL_LOGO``: Path to logo PNG

4. **Standalone Define**: If ``MAKE_STANDALONE`` is set, defines
   ``PORTAL_STANDALONE_EXE`` for the runtime target.

5. **Editor Define**: Defines ``PORTAL_BUILD_EDITOR`` for ``<target_name>_editor``.

6. **Linking**: Links both targets against ``portal::engine`` and any additional libraries.

7. **Settings Configuration** (runtime target only):

   - Resolves and validates the settings file path
   - Sets ``PORTAL_SETTINGS_PATH`` property
   - Calls ``portal_read_settings`` to process settings and set ``PORTAL_DISPLAY_NAME``

8. **macOS Bundle Properties**: If ``MAKE_STANDALONE`` is set, configures bundle
   metadata using the resolved display name.

9. **Engine Assets**: Fetches Portal Engine resources and configs via
   ``portal_fetch_resources`` and ``portal_fetch_configs``.

10. **Local Resources**: Adds user-specified resource paths via
   ``portal_add_resources``.

11. **Local Configs**: Adds the local ``config/`` directory via
    :ref:`portal_add_configs <portal-add-configs>` unless ``NO_CONFIG`` is set.

12. **Config Definitions**: Calls ``portal_setup_compile_configs`` to generate
    compile-time configuration constants for both runtime and editor targets.

13. **Installation**: Calls ``portal_install_game`` to set up installation rules.

14. **Packaging**: Calls ``portal_package_game`` to configure CPack.

Target Properties Set
^^^^^^^^^^^^^^^^^^^^^

The function sets the following target properties on the runtime target:

- ``PORTAL_WINDOWS_ICON``: Windows .ico file path
- ``PORTAL_MACOS_ICON``: macOS .icns file path
- ``PORTAL_LOGO``: Logo PNG file path
- ``PORTAL_SETTINGS_PATH``: Settings JSON file path
- ``PORTAL_DISPLAY_NAME``: Display name parsed from the settings file
- ``PORTAL_RESOURCES``: List of resource directories (via ``portal_read_settings``)
- ``PORTAL_ADDITIONAL_RESOURCES``: Fetched resources (via ``portal_fetch_resources``)

The function also creates ``<target_name>_editor`` and sets:

- ``PORTAL_WINDOWS_ICON``: Windows .ico file path
- ``PORTAL_MACOS_ICON``: macOS .icns file path
- ``PORTAL_LOGO``: Logo PNG file path

The editor target defines ``PORTAL_BUILD_EDITOR`` but does not receive settings,
resource/config copy targets, installation, or packaging.

macOS Bundle Properties (MAKE_STANDALONE only):

- ``MACOSX_BUNDLE``: TRUE
- ``MACOSX_BUNDLE_IDENTIFIER``: com.portal.<target_name>
- ``MACOSX_BUNDLE_BUNDLE_NAME``: Display name
- ``MACOSX_BUNDLE_ICON_FILE``: Icon filename
- ``MACOSX_BUNDLE_BUNDLE_VERSION``: Project version
- ``MACOSX_BUNDLE_SHORT_VERSION_STRING``: Project version

Example Usage
^^^^^^^^^^^^^

Minimal game with default settings:

.. code-block:: cmake

  portal_add_game(my-game
                  SOURCES main.cpp game.cpp)

Full-featured standalone game with custom icons and resources:

.. code-block:: cmake

  portal_add_game(awesome-game
                  SOURCES src/main.cpp src/game.cpp src/player.cpp
                  MAKE_STANDALONE
                  STATIC_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/game_icon"
                  LOGO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/icons/logo.png"
                  SETTINGS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/config/game_settings.json"
                  RESOURCE_PATHS textures models sounds
                  LINK_LIBRARIES my-game-logic-lib
                  VENDOR "Awesome Games Inc."
                  CONTACT "support@awesomegames.com")

This will:

- Create a standalone runtime executable (no console on Windows, bundle on macOS)
- Create a companion ``awesome-game_editor`` console executable for tooling
- Use custom icons (or fallback to defaults)
- Link against portal-engine and my-game-logic-lib
- Fetch Portal Engine resources (shaders, fonts, etc.)
- Add local resources from textures/, models/, and sounds/
- Process and copy game_settings.json
- Set up complete installation and packaging

Notes
^^^^^

- Always links both targets against ``portal::engine``
- Automatically fetches Portal Engine resources for the runtime target
- Settings file is required and must exist
- Icons default to Portal Engine branding if not provided
- ``MAKE_STANDALONE`` is recommended for distribution builds
- Use ``NO_CONFIG`` to opt out of copying a local ``config/`` directory
- The editor target is build-only and not installed or packaged

See Also
^^^^^^^^

- :ref:`portal_setup_compile_configs <portal-setup-compile-configs>`: Generates configuration constants
- :ref:`portal_add_resources <portal-add-resources>`: Add local resource directories
- :ref:`portal_fetch_resources <portal-fetch-resources>`: Fetch resources from other targets
- :ref:`portal_add_configs <portal-add-configs>`: Add local config directory
- :ref:`portal_fetch_configs <portal-fetch-configs>`: Fetch configs from other targets
- :ref:`portal_read_settings <portal-read-settings>`: Process settings file
- :ref:`portal_install_game <portal-install-game>`: Set up installation rules
- :ref:`portal_package_game <portal-package-game>`: Configure CPack packaging
