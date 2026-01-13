.. _portal-build-tests:

Function portal_build_tests
---------------------------

Conditionally builds tests from a subdirectory based on the ``PORTAL_BUILD_TESTS`` option.

This is a convenience function that encapsulates the pattern of checking whether
tests should be built before adding a test subdirectory.

Synopsis
^^^^^^^^

.. code-block:: cmake

  portal_build_tests(<folder_name>)

Arguments
^^^^^^^^^

``<folder_name>``
  Path to the subdirectory containing test code, relative to the current
  source directory. This directory will be added via ``add_subdirectory()``
  if test building is enabled.

Behavior
^^^^^^^^

The function performs the following operations:

1. **Conditional Build**: Checks if the ``PORTAL_BUILD_TESTS`` CMake option is enabled.

2. **Enable Testing**: If tests are enabled, calls ``enable_testing()`` to enable
   CTest integration for the current directory and below.

3. **Add Subdirectory**: Adds the specified folder as a subdirectory if tests are enabled.

4. **Cache Cleanup**: Unsets the ``PORTAL_BUILD_TESTS`` cache variable after checking,
   ensuring clean state for subsequent configurations.

Example Usage
^^^^^^^^^^^^^

Typical usage in a module's CMakeLists.txt:

.. code-block:: cmake

  # In core/CMakeLists.txt
  portal_build_tests(tests)

This will add the ``tests/`` subdirectory only when ``PORTAL_BUILD_TESTS`` is ``ON``.

Notes
^^^^^

- The ``PORTAL_BUILD_TESTS`` option defaults to ``ON`` in the Portal Framework
- Test subdirectories typically contain calls to :ref:`portal_add_test_target <portal-add-test-target>`
- The cache variable is unset after use to prevent interference with other modules
- This function should be called from the module's main CMakeLists.txt, not from
  within the test subdirectory

See Also
^^^^^^^^

- :ref:`portal_add_test_target <portal-add-test-target>`: Creates test executables