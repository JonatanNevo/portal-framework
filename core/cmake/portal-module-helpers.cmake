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

function(portal_add_dependency MODULE_NAME DEPENDENCY)
    set(options PORTAL SKIP_LINK)
    set(oneValueArgs LINK)
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
        if (PORTAL_FIND_PACKAGE)
            find_package(portal-${DEPENDENCY} CONFIG REQUIRED)
        endif ()
    else ()
        find_package(${DEPENDENCY} ${ARG_FIND_PACKAGE_ARGS})
    endif ()

    if (NOT ARG_SKIP_LINK)
        target_link_libraries(portal-${MODULE_NAME} PUBLIC ${ARG_LINK})
    endif ()
endfunction()

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

    get_target_property(COMPLETE_CONFIGs ${TARGET_NAME} PORTAL_CONFIG_HEADERS)

    if (COMPLETE_CONFIGs)
        set(PCH_HEADER_CONTENT "// This is an auto generated file\n#pragma once\n")
        foreach (HEADER ${COMPLETE_CONFIGs})
            string(APPEND PCH_HEADER_CONTENT "#include <${HEADER}>\n")
        endforeach ()

        file(WRITE "${PCH_PATH}" "${PCH_HEADER_CONTENT}")

        target_precompile_headers(${TARGET_NAME} PUBLIC "${PCH_PATH}")
    endif ()


endfunction()

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


function(portal_add_module MODULE_NAME)
    set(options "")
    set(oneValueArgs COMPILE_CONFIG_FILE)
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
    set_target_properties(${TARGET_NAME} PROPERTIES EXPORT_NAME ${MODULE_NAME})
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
        portal_add_dependency(${MODULE_NAME} ${dep} PORTAL)

        get_target_property(CONFIG_FILES portal-${dep} PORTAL_CONFIG_HEADERS)
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