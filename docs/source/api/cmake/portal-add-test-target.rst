.. _portal-add-test-target:

Function portal_add_test_target
-------------------------------

Creates a test executable for a Portal Framework module using Catch2.

This function automates the creation of test targets for Portal modules,
including automatic test discovery and registration with CTest.

Synopsis
^^^^^^^^

.. code-block:: cmake

  portal_add_test_target(<target_name>
                         [SOURCES <source>...]
                         [LIBRARIES <library>...])

Arguments
^^^^^^^^^

``<target_name>``
  Name of the module target to create tests for. The test executable will
  be named ``<target_name>-test``.

``SOURCES <source>...``
  Optional. List of source files for the test executable. If not provided,
  the function automatically glob searches for files matching ``*tests.cpp``
  and ``.h`` files in the current directory and subdirectories.

``LIBRARIES <library>...``
  Optional. Additional libraries to link against the test executable beyond
  the module target itself and Catch2.

Behavior
^^^^^^^^

The function performs the following operations:

1. **Catch2 Setup**: Finds and imports Catch2 (v3+) if not already found.

2. **Testing Enabled**: Calls ``enable_testing()`` to enable CTest integration.

3. **Source Discovery**: If ``SOURCES`` is not specified, automatically finds
   all test source files (``*tests.cpp``) and headers (``.h``) recursively.

4. **Test Executable**: Creates an executable named ``<target_name>-test``
   and links it with:

   - The module target (``<target_name>``)
   - ``Catch2::Catch2`` and ``Catch2::Catch2WithMain``
   - Any additional libraries specified via ``LIBRARIES``

5. **Test Macro**: Defines ``PORTAL_TEST`` on the module target to enable
   test-specific code paths.

6. **Test Discovery**: Uses ``catch_discover_tests()`` to automatically
   register all Catch2 test cases with CTest.

Example Usage
^^^^^^^^^^^^^

Basic test target with automatic source discovery:

.. code-block:: cmake

  portal_add_test_target(portal-core)

Test target with explicit sources and additional libraries:

.. code-block:: cmake

  portal_add_test_target(portal-engine
                         SOURCES engine_tests.cpp renderer_tests.cpp
                         LIBRARIES Vulkan::Vulkan)

Notes
^^^^^

- Requires Catch2 v3+ to be available via ``find_package(Catch2 CONFIG)``
- Test files are expected to follow the naming convention ``*tests.cpp``
- The ``PORTAL_TEST`` define can be used to conditionally compile test-only code
- All discovered tests are automatically registered with CTest

See Also
^^^^^^^^

- :ref:`portal_build_tests <portal-build-tests>`: Conditionally builds tests from a subdirectory