#[=======================================================================[.rst:
portal_register_docs
--------------------

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
- This function is automatically called by ``portal_add_module()`` when
  ``PORTAL_BUILD_DOCS`` is enabled

See Also
^^^^^^^^

- ``portal_add_module``: Main module creation function

#]=======================================================================]
function(portal_register_docs MODULE_NAME)
    set(DOXYFILE_IN_CONTENT "
# Doxyfile 1.15.0

#---------------------------------------------------------------------------
# Project related configuration options
#---------------------------------------------------------------------------

DOXYFILE_ENCODING      = UTF-8
PROJECT_NAME           = @PORTAL_DOXYGEN_PROJECT_NAME@
OUTPUT_DIRECTORY       = @PORTAL_DOXYGEN_OUTPUT@

FULL_PATH_NAMES = YES
STRIP_FROM_PATH = .

ALIASES  = \"rst=\\verbatim embed:rst:leading-asterisk\"
ALIASES += \"endrst=\\endverbatim\"


#---------------------------------------------------------------------------
# Build related configuration options
#---------------------------------------------------------------------------

EXTRACT_ALL            = YES
EXTRACT_STATIC         = YES
EXTRACT_LOCAL_CLASSES  = NO
CASE_SENSE_NAMES       = YES


#---------------------------------------------------------------------------
# Configuration options related to warning and progress messages
#---------------------------------------------------------------------------
WARN_NO_PARAMDOC       = YES


#---------------------------------------------------------------------------
# Configuration options related to the input files
#---------------------------------------------------------------------------
INPUT                  = @PORTAL_DOXYGEN_INPUT@
RECURSIVE              = YES
EXCLUDE                = vcpkg \
                         build \
                         cmake-build-* \
                         out
EXCLUDE_PATTERNS       = */tests/* \
                         */test/* \
                         */.git/* \
                         */vcpkg_installed/* \
                         */build*/* \
                         *_impl.h \
                         *_internal.h
EXCLUDE_SYMBOLS        = detail \
                         internal \
                         impl


#---------------------------------------------------------------------------
# Configuration options related to the HTML output
#---------------------------------------------------------------------------
GENERATE_HTML          = NO
GENERATE_LATEX         = NO

#---------------------------------------------------------------------------
# Configuration options related to the XML output
#---------------------------------------------------------------------------
GENERATE_XML           = YES

#---------------------------------------------------------------------------
# Configuration options related to the preprocessor
#---------------------------------------------------------------------------
ENABLE_PREPROCESSING   = YES
MACRO_EXPANSION        = YES
INCLUDE_PATH           = @PORTAL_DOXYGEN_INCLUDE_PATH@
SKIP_FUNCTION_MACROS   = NO
EXPAND_ONLY_PREDEF     = NO

PREDEFINED             = DOXYGEN_DOCUMENTATION_BUILD
PREDEFINED             += DOXYGEN_SHOULD_SKIP_THIS
")

    set(PORTAL_DOXYGEN_PROJECT_NAME "portal ${MODULE_NAME}")
    set(PORTAL_DOXYGEN_OUTPUT ${PORTAL_DOCS_BINARY_DIR}/doxygen/${MODULE_NAME})
    set(PORTAL_DOXYGEN_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/portal)
    set(PORTAL_DOXYGEN_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

    file(MAKE_DIRECTORY ${PORTAL_DOXYGEN_OUTPUT})

    set(DOXYFILE_INPUT "${CMAKE_CURRENT_BINARY_DIR}/docs/Doxyfile.in")
    set(DOXYFILE_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/docs/Doxyfile")

    file(WRITE "${DOXYFILE_INPUT}" "${DOXYFILE_IN_CONTENT}")

    configure_file(${DOXYFILE_INPUT} ${DOXYFILE_OUTPUT} @ONLY)

    set(GENERATE_DOCS_TARGET "${MODULE_NAME}_make_docs")

    add_custom_target(${GENERATE_DOCS_TARGET}
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUTPUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            DEPENDS ${DOXYFILE_OUTPUT}
            COMMENT "Generating Doxygen XML for ${MODULE_NAME}"
            VERBATIM
    )
    set_property(GLOBAL APPEND PROPERTY PORTAL_DOCS_TARGETS ${GENERATE_DOCS_TARGET})
endfunction()

#[=======================================================================[.rst:
portal_add_dependency
---------------------

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

- ``portal_add_module``: Main module creation function that uses this internally

#]=======================================================================]
function(portal_add_dependency MODULE_NAME DEPENDENCY)
    set(options PORTAL SKIP_LINK)
    set(oneValueArgs LINK PORTAL_FIND_PACKAGE)
    set(multiValueArgs FIND_PACKAGE_ARGS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT ARG_LINK)
        if (ARG_PORTAL)
            set(ARG_LINK portal::${DEPENDENCY})
        else ()
            set(ARG_LINK ${DEPENDENCY}::${DEPENDENCY})
        endif ()
    endif ()

    if (NOT ARG_FIND_PACKAGE_ARGS)
        set(ARG_FIND_PACKAGE_ARGS CONFIG REQUIRED)
    endif ()

    if (ARG_PORTAL)
        if (ARG_PORTAL_FIND_PACKAGE)
            find_package(portal-${DEPENDENCY} CONFIG REQUIRED)
        endif ()
    else ()
        find_package(${DEPENDENCY} ${ARG_FIND_PACKAGE_ARGS})
    endif ()

    if (NOT ARG_SKIP_LINK)
        target_link_libraries(portal-${MODULE_NAME} PUBLIC ${ARG_LINK})
    endif ()
endfunction()

#[=======================================================================[.rst:
portal_setup_config_pch
-----------------------

Sets up configuration precompiled headers for a Portal module.

This function generates and configures a precompiled header that includes
all configuration headers from the module and its Portal dependencies.

Synopsis
^^^^^^^^

.. code-block:: cmake

  portal_setup_config_pch(<module_name>
                          [GAME]
                          [COMPILE_CONFIG_FILE <file>]
                          [DEPENDENT_CONFIG_HEADERS <header>...])

Arguments
^^^^^^^^^

``<module_name>``
  Name of the module (without ``portal-`` prefix) to set up config PCH for.

``GAME``
  Optional flag indicating this is a game target rather than a Portal module.
  Changes the target name from ``portal-<module_name>`` to just ``<module_name>``
  and adjusts the PCH output path accordingly.

``COMPILE_CONFIG_FILE <file>``
  Optional. Path to the module's own configuration header file. This will be
  added to the ``PORTAL_CONFIG_HEADERS`` property and included in the generated PCH.

``DEPENDENT_CONFIG_HEADERS <header>...``
  Optional. List of configuration headers from Portal dependencies. These are
  typically collected from ``PORTAL_CONFIG_HEADERS`` properties of dependent modules.

Behavior
^^^^^^^^

The function performs the following operations:

1. **Property Setup**: Sets the ``PORTAL_CONFIG_HEADERS`` target property with
   all config headers (both dependent and module-specific) and marks it for export.

2. **PCH Generation**: Creates an auto-generated precompiled header file at:

   - For Portal modules: ``${CMAKE_CURRENT_BINARY_DIR}/portal/<module_name>/config_pch.h``
   - For game targets (``GAME`` flag): ``${CMAKE_CURRENT_BINARY_DIR}/<module_name>/config_pch.h``

3. **PCH Content**: The generated header includes:

   - ``#pragma once`` guard
   - ``#include`` directives for all configuration headers
   - Auto-generation comment

4. **Target Configuration**: Adds the generated header as a PUBLIC precompiled
   header to the target, making it available to all dependent targets.

Example Usage
^^^^^^^^^^^^^

Setting up config PCH with module's own config file:

.. code-block:: cmake

  portal_setup_config_pch(core
                          COMPILE_CONFIG_FILE portal/core/config.h)

Setting up config PCH with dependent configuration headers:

.. code-block:: cmake

  portal_setup_config_pch(engine
                          COMPILE_CONFIG_FILE portal/engine/config.h
                          DEPENDENT_CONFIG_HEADERS
                              portal/core/config.h
                              portal/application/config.h)

Setting up for a game target:

.. code-block:: cmake

  portal_setup_config_pch(my_game
                          GAME
                          DEPENDENT_CONFIG_HEADERS portal/engine/config.h)

Notes
^^^^^

- The ``PORTAL_CONFIG_HEADERS`` property is exported to allow consumers to access
  configuration headers when the module is imported
- Config headers from dependencies are automatically collected by ``portal_add_module()``
- The generated PCH is PUBLIC, propagating to all dependent targets
- This function is automatically called by ``portal_add_module()``

See Also
^^^^^^^^

- ``portal_add_module``: Main module creation function that calls this
- ``portal_configure_pch``: Alternative PCH configuration for non-config headers

#]=======================================================================]
function(portal_setup_config_pch MODULE_NAME)
    set(options GAME)
    set(oneValueArgs COMPILE_CONFIG_FILE)
    set(multiValueArgs DEPENDENT_CONFIG_HEADERS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(TARGET_NAME portal-${MODULE_NAME})
    set(PCH_PATH ${CMAKE_CURRENT_BINARY_DIR}/portal/${MODULE_NAME}/config_pch.h)

    if (ARG_GAME)
        set(TARGET_NAME ${MODULE_NAME})
        set(PCH_PATH ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}/config_pch.h)
    endif ()

    set_property(TARGET ${TARGET_NAME} APPEND PROPERTY PORTAL_CONFIG_HEADERS ${ARG_DEPENDENT_CONFIG_HEADERS})
    if (ARG_COMPILE_CONFIG_FILE)
        set_property(TARGET ${TARGET_NAME} APPEND PROPERTY PORTAL_CONFIG_HEADERS ${ARG_COMPILE_CONFIG_FILE})
    endif ()
    set_property(TARGET ${TARGET_NAME} APPEND PROPERTY EXPORT_PROPERTIES PORTAL_CONFIG_HEADERS)

    get_target_property(COMPLETE_CONFIGS ${TARGET_NAME} PORTAL_CONFIG_HEADERS)

    if (COMPLETE_CONFIGS)
        set(PCH_HEADER_CONTENT "// This is an auto generated file\n#pragma once\n")
        foreach (HEADER ${COMPLETE_CONFIGS})
            string(APPEND PCH_HEADER_CONTENT "#include <${HEADER}>\n")
        endforeach ()

        file(WRITE "${PCH_PATH}" "${PCH_HEADER_CONTENT}")

        target_precompile_headers(${TARGET_NAME} PUBLIC "${PCH_PATH}")
    endif ()
endfunction()

#[=======================================================================[.rst:
portal_configure_pch
--------------------

Configures precompiled headers for a target with optional CMake variable substitution.

This macro is typically used for setting up module-specific precompiled headers
that may need configuration-time processing (e.g., ``.inc`` files with CMake variables).

Synopsis
^^^^^^^^

.. code-block:: cmake

  portal_configure_pch(<target_name> <precompiled_headers>
                       [CONFIGURE])

Arguments
^^^^^^^^^

``<target_name>``
  Name of the target to add precompiled headers to. Can be any CMake target,
  not just Portal modules.

``<precompiled_headers>``
  List of header files to use as precompiled headers. Can be paths relative
  to ``CMAKE_CURRENT_SOURCE_DIR``.

``CONFIGURE``
  Optional flag indicating that headers should be processed through
  ``configure_file()`` before being used as PCH. Useful for headers containing
  CMake variable references (``@VAR@`` or ``${VAR}``).

Behavior
^^^^^^^^

The function performs the following operations for each header:

1. **Header Processing**:

   - If ``CONFIGURE`` is set:
     - Strips ``.inc`` suffix from the filename
     - Processes the header through ``configure_file()`` to substitute CMake variables
     - Outputs configured header to ``${CMAKE_CURRENT_BINARY_DIR}``
   - If ``CONFIGURE`` is not set:
     - Uses the header as-is from ``${CMAKE_CURRENT_SOURCE_DIR}``

2. **PCH Setup**: Adds the header (configured or original) as a PUBLIC
   precompiled header to the target.

3. **File Set Addition**: Adds the header to the target's ``additional_headers``
   file set, making it available for installation and export.

Example Usage
^^^^^^^^^^^^^

Using headers as-is (no configuration):

.. code-block:: cmake

  portal_configure_pch(portal-core
                       portal/core/pch.h)

Using headers with CMake variable substitution:

.. code-block:: cmake

  # portal/core/config.h.inc contains CMake variables like @PROJECT_VERSION@
  portal_configure_pch(portal-core
                       portal/core/config.h.inc
                       CONFIGURE)
  # Generates: ${CMAKE_CURRENT_BINARY_DIR}/portal/core/config.h

Multiple headers with configuration:

.. code-block:: cmake

  set(PCH_HEADERS
      portal/core/config.h.inc
      portal/core/platform.h.inc
  )
  portal_configure_pch(portal-core ${PCH_HEADERS} CONFIGURE)

Notes
^^^^^

- Configured headers have the ``.inc`` extension automatically removed
- All headers are added as PUBLIC PCH, propagating to dependent targets
- Headers are added to the ``additional_headers`` file set for proper installation

See Also
^^^^^^^^

- ``portal_setup_config_pch``: Generates and configures config PCH automatically
- ``portal_add_module``: Main module creation function

#]=======================================================================]
macro(portal_configure_pch TARGET_NAME PRECOMPILED_HEADERS)
    set(options CONFIGURE)
    set(oneValueArgs "")
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    foreach(header ${PRECOMPILED_HEADERS})
        if (ARG_CONFIGURE)
            string(REGEX REPLACE "\\.inc$" "" header_out "${header}")
            set(header_path "${CMAKE_CURRENT_BINARY_DIR}/${header_out}")
            configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${header}" ${header_path})
        else()
            set(header_path "${CMAKE_CURRENT_SOURCE_DIR}/${header}")
        endif()

        target_precompile_headers(${TARGET_NAME} PUBLIC "${header_path}")

        target_sources(${TARGET_NAME} PUBLIC
                FILE_SET additional_headers
                TYPE HEADERS
                BASE_DIRS ${CMAKE_CURRENT_BINARY_DIR}
                FILES ${header_path}
        )
    endforeach ()
endmacro()


#[=======================================================================[.rst:
portal_add_module
-----------------

Creates a Portal Framework module with full configuration including dependencies,
platform-specific sources, precompiled headers, and documentation.

This is the main entry point for creating Portal modules. It orchestrates all
aspects of module creation and configuration.

Synopsis
^^^^^^^^

.. code-block:: cmake

  portal_add_module(<module_name>
                    SOURCES <source>...
                    HEADERS <header>...
                    [WINDOWS_SOURCES <source>...]
                    [WINDOWS_HEADERS <header>...]
                    [MACOS_SOURCES <source>...]
                    [MACOS_HEADERS <header>...]
                    [LINUX_SOURCES <source>...]
                    [LINUX_HEADERS <header>...]
                    [COMPILE_CONFIG_FILE <file>]
                    [PORTAL_FIND_PACKAGE <bool>]
                    [PORTAL_DEPENDENCIES <dep>...]
                    [DEPENDENCIES <dep>...]
                    [COMPLEX_DEPENDENCIES <dep_spec>...])

Arguments
^^^^^^^^^

``<module_name>``
  Name of the module (without ``portal-`` prefix). The function creates a
  target named ``portal-<module_name>`` and an alias ``portal::<module_name>``.

``SOURCES <source>...``
  Required. List of source files (.cpp, .cc, etc.) for the module.

``HEADERS <header>...``
  Required. List of public header files to install and export.

``WINDOWS_SOURCES <source>...``
  Optional. Windows-specific source files (used only when building on Windows).

``WINDOWS_HEADERS <header>...``
  Optional. Windows-specific public headers (used only when building on Windows).

``MACOS_SOURCES <source>...``
  Optional. macOS-specific source files (used only when building on macOS).

``MACOS_HEADERS <header>...``
  Optional. macOS-specific public headers (used only when building on macOS).

``LINUX_SOURCES <source>...``
  Optional. Linux-specific source files (used only when building on Linux).

``LINUX_HEADERS <header>...``
  Optional. Linux-specific public headers (used only when building on Linux).

``COMPILE_CONFIG_FILE <file>``
  Optional. Path to the module's configuration header (e.g., ``portal/core/config.h``).
  This is included in the generated config PCH and exported for dependent modules.

``PORTAL_FIND_PACKAGE <bool>``
  Optional. Whether to call ``find_package()`` for Portal dependencies. Set to
  ``ON`` when consuming installed Portal modules, ``OFF`` for in-tree builds.

``PORTAL_DEPENDENCIES <dep>...``
  Optional. List of Portal module dependencies (names without ``portal-`` prefix).
  These are linked with ``portal::<dep>`` and their config headers are collected.

``DEPENDENCIES <dep>...``
  Optional. List of external library dependencies. Each is found via
  ``find_package()`` and linked with ``<dep>::<dep>``.

``COMPLEX_DEPENDENCIES <dep_spec>...``
  Optional. List of dependencies with custom options. Each spec is formatted as
  ``<dep_name>|<arg1>|<arg2>...`` where args are passed to ``portal_add_dependency()``.

Behavior
^^^^^^^^

The function performs the following operations:

1. **Target Creation**: Creates a STATIC library named ``portal-<module_name>``
   with C++23 standard and creates an alias ``portal::<module_name>``.

2. **File Sets**: Adds sources and headers with proper file sets for installation.

3. **Platform-Specific Sources**: Conditionally adds platform-specific sources
   and headers based on the build platform (Windows, macOS, or Linux).

4. **Compiler Flags**:

   - MSVC: ``/W4 /WX`` (warning level 4, warnings as errors)
   - Clang/GCC: ``-Wall -Wextra -Wpedantic -Werror -Wno-missing-designated-field-initializers``

5. **Dependency Resolution**: Processes all dependencies via ``portal_add_dependency()``:

   - External dependencies from ``DEPENDENCIES``
   - Portal dependencies from ``PORTAL_DEPENDENCIES``
   - Complex dependencies from ``COMPLEX_DEPENDENCIES``

6. **Config PCH Setup**: Calls ``portal_setup_config_pch()`` to generate and
   configure precompiled headers with all config files from dependencies.

7. **Documentation**: If ``PORTAL_BUILD_DOCS`` is enabled, calls
   ``portal_register_docs()`` to set up Doxygen documentation generation.

Target Properties
^^^^^^^^^^^^^^^^^

The created target has the following properties:

- ``EXPORT_NAME``: Set to ``<module_name>`` (without ``portal-`` prefix)
- ``CXX_STANDARD``: 23
- ``CXX_STANDARD_REQUIRED``: ON
- ``CXX_EXTENSIONS``: OFF
- ``PORTAL_CONFIG_HEADERS``: List of configuration headers (exported)

Example Usage
^^^^^^^^^^^^^

Basic module with external dependencies:

.. code-block:: cmake

  portal_add_module(core
      SOURCES
          src/log.cpp
          src/memory.cpp
      HEADERS
          portal/core/log.h
          portal/core/memory.h
      COMPILE_CONFIG_FILE
          portal/core/config.h
      DEPENDENCIES
          spdlog
          glm
          mimalloc
  )

Module with Portal dependencies:

.. code-block:: cmake

  portal_add_module(engine
      SOURCES
          src/renderer.cpp
          src/scene.cpp
      HEADERS
          portal/engine/renderer.h
          portal/engine/scene.h
      COMPILE_CONFIG_FILE
          portal/engine/config.h
      PORTAL_DEPENDENCIES
          core
          application
          input
      DEPENDENCIES
          Vulkan
          imgui
  )

Module with platform-specific code:

.. code-block:: cmake

  portal_add_module(core
      SOURCES
          src/common.cpp
      HEADERS
          portal/core/common.h
      WINDOWS_SOURCES
          src/windows/file_system.cpp
      WINDOWS_HEADERS
          portal/platform/core/windows/file_system.h
      MACOS_SOURCES
          src/macos/file_system.mm
      MACOS_HEADERS
          portal/platform/core/macos/file_system.h
      LINUX_SOURCES
          src/linux/file_system.cpp
      LINUX_HEADERS
          portal/platform/core/linux/file_system.h
  )

Module with complex dependencies:

.. code-block:: cmake

  portal_add_module(networking
      SOURCES
          src/network.cpp
      HEADERS
          portal/networking/network.h
      PORTAL_DEPENDENCIES
          core
      COMPLEX_DEPENDENCIES
          "GameNetworkingSockets|LINK|GameNetworkingSockets::GameNetworkingSockets"
          "OpenSSL|FIND_PACKAGE_ARGS|REQUIRED|COMPONENTS|SSL|Crypto"
  )

Notes
^^^^^

- All modules are built as STATIC libraries
- All dependencies are linked as PUBLIC to propagate to consumers
- The module automatically participates in documentation generation when enabled
- Config headers from Portal dependencies are automatically collected and included
  in the generated config PCH

See Also
^^^^^^^^

- ``portal_add_dependency``: Handles individual dependency configuration
- ``portal_setup_config_pch``: Sets up configuration precompiled headers
- ``portal_register_docs``: Registers module for documentation generation
- ``portal_install_module``: Installs the module with CMake package config

#]=======================================================================]
function(portal_add_module MODULE_NAME)
    set(options "")
    set(oneValueArgs COMPILE_CONFIG_FILE PORTAL_FIND_PACKAGE)
    set(multiValueArgs
            SOURCES
            HEADERS
            WINDOWS_SOURCES
            WINDOWS_HEADERS
            MACOS_SOURCES
            MACOS_HEADERS
            LINUX_SOURCES
            LINUX_HEADERS
            PORTAL_DEPENDENCIES
            DEPENDENCIES
            COMPLEX_DEPENDENCIES
    )
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(TARGET_NAME portal-${MODULE_NAME})
    message(STATUS "Adding module - ${TARGET_NAME}")

    add_library(${TARGET_NAME} STATIC ${ARG_SOURCES})
    target_sources(${TARGET_NAME} PUBLIC
            FILE_SET HEADERS
            BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
            FILES ${ARG_HEADERS}
    )
    set_target_properties(${TARGET_NAME} PROPERTIES
            EXPORT_NAME ${MODULE_NAME}
            CXX_STANDARD 23
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
    )
    add_library(portal::${MODULE_NAME} ALIAS ${TARGET_NAME})

    if (WIN32)
        if (ARG_WINDOWS_SOURCES OR ARG_WINDOWS_HEADERS)
            target_sources(${TARGET_NAME}
                    PRIVATE ${ARG_WINDOWS_SOURCES}
                    PUBLIC
                    FILE_SET HEADERS
                    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                    FILES ${ARG_WINDOWS_HEADERS}
            )
        endif ()
    elseif (APPLE)
        if (ARG_MACOS_SOURCES OR ARG_MACOS_HEADERS)
            target_sources(${TARGET_NAME}
                    PRIVATE ${ARG_MACOS_SOURCES}
                    PUBLIC
                    FILE_SET HEADERS
                    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                    FILES ${ARG_MACOS_HEADERS}
            )
        endif ()
    elseif (UNIX)
        if (ARG_LINUX_SOURCES OR ARG_LINUX_HEADERS)
            target_sources(${TARGET_NAME}
                    PRIVATE ${ARG_LINUX_SOURCES}
                    PUBLIC
                    FILE_SET HEADERS
                    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                    FILES ${ARG_LINUX_HEADERS}
            )
        endif ()
    else ()
        message(FATAL_ERROR "Unsupported platform")
    endif ()

    if (MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
    else ()
        target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-missing-designated-field-initializers)
    endif ()

    foreach (dep ${ARG_DEPENDENCIES})
        portal_add_dependency(${MODULE_NAME} ${dep})
    endforeach ()

    set(DEPENDENT_CONFIG_HEADERS "")
    foreach (dep ${ARG_PORTAL_DEPENDENCIES})
        portal_add_dependency(${MODULE_NAME} ${dep}
                PORTAL
                PORTAL_FIND_PACKAGE ${ARG_PORTAL_FIND_PACKAGE}
        )

        if (${ARG_PORTAL_FIND_PACKAGE})
            get_target_property(CONFIG_FILES portal::${dep} PORTAL_CONFIG_HEADERS)
        else ()
            get_target_property(CONFIG_FILES portal-${dep} PORTAL_CONFIG_HEADERS)
        endif ()

        if (CONFIG_FILES)
            list(APPEND DEPENDENT_CONFIG_HEADERS ${CONFIG_FILES})
        endif ()
    endforeach ()
    list(REMOVE_DUPLICATES DEPENDENT_CONFIG_HEADERS)

    foreach (dep_spec ${ARG_COMPLEX_DEPENDENCIES})
        string(REPLACE "|" " " dep_args "${dep_spec}")
        separate_arguments(dep_args)
        list(GET dep_args 0 dep_name)
        list(REMOVE_AT dep_args 0)

        portal_add_dependency(${MODULE_NAME} ${dep_name} ${dep_args})
    endforeach ()
    unset(PORTAL_FIND_PACKAGE CACHE)

    portal_setup_config_pch(
            ${MODULE_NAME}
            COMPILE_CONFIG_FILE
                ${ARG_COMPILE_CONFIG_FILE}
            DEPENDENT_CONFIG_HEADERS
                ${DEPENDENT_CONFIG_HEADERS}
    )

    if (PORTAL_BUILD_DOCS)
        portal_register_docs(${MODULE_NAME})
    endif ()
endfunction()