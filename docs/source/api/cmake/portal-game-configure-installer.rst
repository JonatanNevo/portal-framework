.. _portal-game-configure-installer:

Function portal_game_configure_installer
----------------------------------------

Configures Qt Installer Framework (IFW) settings for creating cross-platform installers
for Portal Engine games.

This function sets up Qt IFW-specific configuration to generate professional installers
with custom branding (icons, logos) and metadata. It works in conjunction with CPack to
create platform-specific installer packages.

Synopsis
^^^^^^^^

.. code-block:: cmake

  portal_game_configure_installer(<target_name>
                                   [URL <url>])

Arguments
^^^^^^^^^

``<target_name>``
  Name of the game executable target. The function will read branding properties
  (``PORTAL_DISPLAY_NAME``, ``PORTAL_WINDOWS_ICON``, ``PORTAL_MACOS_ICON``,
  ``PORTAL_LOGO``) from this target.

``URL <url>``
  Optional. Product/project URL for the installer (e.g., game website, GitHub repository).
  If not specified, defaults to ``https://github.com/JonatanNevo/portal-framework``.

Behavior
^^^^^^^^

The function performs the following operations:

1. **Display Name Retrieval**: Reads the ``PORTAL_DISPLAY_NAME`` property from the
   target to use as the installer title.

2. **URL Configuration**: Sets the product URL to the provided value or the default
   Portal Framework repository.

3. **IFW Package Configuration**: Sets core Qt IFW variables:

   - ``CPACK_IFW_VERBOSE``: Enabled for detailed installer generation output
   - ``CPACK_IFW_PACKAGE_TITLE``: Set to the game's display name
   - ``CPACK_IFW_PRODUCT_URL``: Set to the product URL
   - ``CPACK_IFW_PACKAGE_WIZARD_STYLE``: Set to "Modern" for contemporary UI

4. **Maintenance Tool Configuration**: Configures the uninstaller with a custom name:

   - ``CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_NAME``: Set to ``<project_name>_MaintenanceTool``
   - ``CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_INI_FILE``: Corresponding .ini file name

5. **Branding Assets**: Retrieves icon and logo properties from the target and
   sets platform-specific installer icons:

   - **Windows**: Uses ``PORTAL_WINDOWS_ICON`` (.ico file)
   - **macOS**: Uses ``PORTAL_MACOS_ICON`` (.icns file)
   - **Logo**: Uses ``PORTAL_LOGO`` for installer branding (all platforms)

6. **CPackIFW Inclusion**: Includes the CPack IFW module to activate installer generation.

IFW Variables Set
^^^^^^^^^^^^^^^^^

The function configures the following CPack IFW variables:

- ``CPACK_IFW_VERBOSE``: ON
- ``CPACK_IFW_PACKAGE_TITLE``: Game display name
- ``CPACK_IFW_PRODUCT_URL``: Product/project URL
- ``CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_NAME``: Custom maintenance tool name
- ``CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_INI_FILE``: Maintenance tool config file
- ``CPACK_IFW_PACKAGE_WIZARD_STYLE``: "Modern"
- ``CPACK_IFW_PACKAGE_ICON``: Platform-specific icon (.ico on Windows, .icns on macOS)
- ``CPACK_IFW_PACKAGE_WINDOWS_LOGO``: Logo for Windows installer
- ``CPACK_IFW_PACKAGE_LOGO``: Logo for installer (all platforms)

Example Usage
^^^^^^^^^^^^^

Basic installer configuration with default URL:

.. code-block:: cmake

  portal_game_configure_installer(my-game)

Installer with custom project URL:

.. code-block:: cmake

  portal_game_configure_installer(awesome-game
                                   URL "https://www.awesomegames.com")


This creates a Qt IFW-based installer in the build directory.


Notes
^^^^^

- Requires Qt Installer Framework (IFW) to be installed on the build machine
- Target properties (``PORTAL_DISPLAY_NAME``, icons, logo) must be set before calling
- The maintenance tool allows users to modify, update, or uninstall the application

See Also
^^^^^^^^

- :ref:`portal_add_game <portal-add-game>`: Creates game targets and sets required properties
- :ref:`portal_package_game <portal-package-game>`: Configures CPack packaging (prerequisite)
- CPack IFW Generator: https://cmake.org/cmake/help/latest/cpack_gen/ifw.html