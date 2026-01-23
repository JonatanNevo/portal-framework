#[=======================================================================[.rst:
portal_add_test_target
-----------------------

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

#]=======================================================================]
function(portal_add_test_target TARGET_NAME)
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs SOURCES LIBRARIES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT Catch2_FOUND)
        find_package(Catch2 CONFIG REQUIRED)
    endif()

    enable_testing()

    if(NOT ARG_SOURCES)
        file(GLOB_RECURSE TEST_SOURCES "*tests.cpp")
        file(GLOB_RECURSE TEST_HEADERS "*.h")
        set(ARG_SOURCES ${TEST_SOURCES} ${TEST_HEADERS})
    endif()

    set(TEST_TARGET ${TARGET_NAME}-test)
    add_executable(${TEST_TARGET} ${ARG_SOURCES})

    target_link_libraries(${TEST_TARGET}
            PRIVATE
            ${TARGET_NAME}
            Catch2::Catch2
            Catch2::Catch2WithMain
            ${ARG_LIBRARIES}
    )

    target_compile_definitions(${TEST_TARGET} PUBLIC PORTAL_TEST)

    include(Catch)
    catch_discover_tests(${TEST_TARGET})
endfunction()

#[=======================================================================[.rst:
portal_build_tests
------------------

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
- Test subdirectories typically contain calls to ``portal_add_test_target()``
- The cache variable is unset after use to prevent interference with other modules
- This function should be called from the module's main CMakeLists.txt, not from
  within the test subdirectory

See Also
^^^^^^^^

- ``portal_add_test_target``: Creates test executables

#]=======================================================================]
function(portal_build_tests FOLDER_NAME)
    if (PORTAL_BUILD_TESTS)
        enable_testing()
        add_subdirectory(${FOLDER_NAME})
    endif ()
    unset(PORTAL_BUILD_TESTS CACHE)
endfunction()