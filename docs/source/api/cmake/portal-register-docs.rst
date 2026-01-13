.. _portal-register-docs:

Function portal_register_docs
-----------------------------

Registers a Portal module for Doxygen XML documentation generation.

This function creates a custom target that generates Doxygen XML output for
a module, which is later consumed by Sphinx to build the final documentation.

Synopsis
^^^^^^^^

.. code-block:: cmake

  portal_register_docs(<module_name>)

Arguments
^^^^^^^^^

``<module_name>``
  Name of the module (without the ``portal-`` prefix) to generate documentation for.

Behavior
^^^^^^^^

The function performs the following operations:

1. **Doxyfile Generation**: Creates a custom Doxyfile configuration from an
   embedded template with the following settings:

   - Extracts all entities (classes, functions, etc.)
   - Generates XML output only (no HTML/LaTeX)
   - Enables preprocessing and macro expansion
   - Excludes test files, internal implementations, and build directories
   - Defines ``DOXYGEN_DOCUMENTATION_BUILD`` for conditional documentation

2. **Custom Target**: Creates a target named ``<module_name>_make_docs`` that
   runs Doxygen when built.

3. **Global Registration**: Appends the target to the ``PORTAL_DOCS_TARGETS``
   global property for aggregation in the main documentation build.

Configuration
^^^^^^^^^^^^^

The generated Doxyfile uses the following paths:

- **Input**: ``${CMAKE_CURRENT_SOURCE_DIR}/portal`` (module headers)
- **Output**: ``${PORTAL_DOCS_BINARY_DIR}/doxygen/<module_name>``
- **Include path**: ``${CMAKE_CURRENT_SOURCE_DIR}``

Excluded Patterns
^^^^^^^^^^^^^^^^^

The following files and directories are excluded from documentation:

- Test directories (``*/tests/*``, ``*/test/*``)
- Internal implementation files (``*_impl.h``, ``*_internal.h``)
- Build directories (``build``, ``cmake-build-*``, ``out``)
- vcpkg and git directories
- Namespaces: ``detail``, ``internal``, ``impl``

Example Usage
^^^^^^^^^^^^^

Typical usage in a module's CMakeLists.txt:

.. code-block:: cmake

  if (PORTAL_BUILD_DOCS)
      portal_register_docs(core)
  endif()

Notes
^^^^^

- Requires ``DOXYGEN_EXECUTABLE`` to be set (found via ``find_package(Doxygen)``)
- Only generates XML output for consumption by Sphinx
- The RST alias enables embedded reStructuredText in Doxygen comments
- This function is automatically called by :ref:`portal_add_module <portal-add-module>` when
  ``PORTAL_BUILD_DOCS`` is enabled

See Also
^^^^^^^^

- :ref:`portal_add_module <portal-add-module>`: Main module creation function