.. _portal-add-dependency:

Function portal_add_dependency
------------------------------

Adds a dependency to a Portal module, handling both Portal and external dependencies.

This function abstracts the process of finding and linking dependencies,
with special handling for Portal Framework modules.

Synopsis
^^^^^^^^

.. code-block:: cmake

  portal_add_dependency(<module_name> <dependency>
                        [PORTAL]
                        [SKIP_LINK]
                        [LINK <target>]
                        [PORTAL_FIND_PACKAGE]
                        [FIND_PACKAGE_ARGS <arg>...])

Arguments
^^^^^^^^^

``<module_name>``
  Name of the Portal module (without ``portal-`` prefix) that depends on the dependency.

``<dependency>``
  Name of the dependency to add.

``PORTAL``
  Optional flag indicating this is a Portal Framework module dependency.
  Changes the default link target to ``portal::<dependency>`` and alters
  find_package behavior.

``SKIP_LINK``
  Optional flag to skip the ``target_link_libraries`` call. Useful when you
  only need to find the package but not link against it.

``LINK <target>``
  Optional. Explicitly specify the link target name. If not provided:

  - For Portal dependencies (``PORTAL`` flag): defaults to ``portal::<dependency>``
  - For external dependencies: defaults to ``<dependency>::<dependency>``

``PORTAL_FIND_PACKAGE``
  Optional flag indicating that ``find_package`` should be called for a Portal
  dependency. Only used when ``PORTAL`` is set.

``FIND_PACKAGE_ARGS <arg>...``
  Optional arguments to pass to ``find_package()``. Defaults to ``CONFIG REQUIRED``.

Behavior
^^^^^^^^

The function performs the following operations:

1. **Link Target Determination**: Determines the appropriate link target based
   on whether it's a Portal or external dependency.

2. **Package Finding**:

   - For Portal dependencies with ``PORTAL_FIND_PACKAGE``: calls
     ``find_package(portal-<dependency> CONFIG REQUIRED)``
   - For Portal dependencies without ``PORTAL_FIND_PACKAGE``: skips find_package
     (assumes in-tree build)
   - For external dependencies: calls ``find_package(<dependency> <args>)``

3. **Linking**: Links the dependency to ``portal-<module_name>`` as a PUBLIC
   dependency, unless ``SKIP_LINK`` is specified.

Example Usage
^^^^^^^^^^^^^

Adding an external dependency:

.. code-block:: cmake

  portal_add_dependency(core spdlog)
  # Calls: find_package(spdlog CONFIG REQUIRED)
  # Links: target_link_libraries(portal-core PUBLIC spdlog::spdlog)

Adding a Portal module dependency (in-tree):

.. code-block:: cmake

  portal_add_dependency(engine core PORTAL)
  # Skips find_package (assumes in-tree)
  # Links: target_link_libraries(portal-engine PUBLIC portal::core)

Adding a Portal module dependency (installed):

.. code-block:: cmake

  portal_add_dependency(engine core PORTAL PORTAL_FIND_PACKAGE)
  # Calls: find_package(portal-core CONFIG REQUIRED)
  # Links: target_link_libraries(portal-engine PUBLIC portal::core)

Custom link target and find_package arguments:

.. code-block:: cmake

  portal_add_dependency(engine Vulkan
                        LINK Vulkan::Vulkan
                        FIND_PACKAGE_ARGS REQUIRED COMPONENTS Vulkan)

Notes
^^^^^

- All dependencies are linked as PUBLIC to propagate to dependent targets

See Also
^^^^^^^^

- :ref:`portal_add_module <portal-add-module>`: Main module creation function that uses this internally