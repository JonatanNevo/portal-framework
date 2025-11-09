# Set the script path once when this file is included
set(PORTAL_PREPARE_CONSTEVAL_STRINGS_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/prepare_consteval_strings.py" CACHE STRING "")
set(PORTAL_MAKE_FROZEN_MAP_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/make_frozen_map.py" CACHE STRING "")

# Global list to track all generated inc files
set_property(GLOBAL PROPERTY PORTAL_GENERATED_INC_FILES "")

include(${CMAKE_CURRENT_LIST_DIR}/rapidhash_runner.cmake)

function(prepare_consteval_strings TARGET_NAME BASE_DIR OUTPUT_PATH)
    find_package(Python3 COMPONENTS Interpreter)
    if (Python3_FOUND)
        message(STATUS "Python 3 interpreter found at: ${Python3_EXECUTABLE}")
    else()
        message(FATAL_ERROR "Python 3 not found. Please install Python 3 or specify its location.")
    endif()
    # Use the globally set script path
    set(SCRIPT_PATH "${PORTAL_PREPARE_CONSTEVAL_STRINGS_SCRIPT}")

    # Collect all relevant files from BASE_DIR to track as dependencies
    # Adjust the patterns based on what file types your script processes
    file(GLOB_RECURSE BASE_DIR_FILES
            "${BASE_DIR}/*.h"
            "${BASE_DIR}/*.hpp"
            "${BASE_DIR}/*.cpp"
            "${BASE_DIR}/*.cc"
            "${BASE_DIR}/*.cxx"
    )

    # Create a unique custom target name
    set(CUSTOM_TARGET_NAME "${TARGET_NAME}_prepare_consteval_strings")

    # Add custom target that runs the Python script
    add_custom_command(
            OUTPUT ${OUTPUT_PATH}
            COMMAND ${Python3_EXECUTABLE} ${SCRIPT_PATH} ${BASE_DIR} ${OUTPUT_PATH} $<TARGET_FILE:rapidhash_runner>
            DEPENDS ${BASE_DIR_FILES} ${SCRIPT_PATH} rapidhash_runner
            COMMENT "Preparing consteval strings from ${BASE_DIR} to ${OUTPUT_PATH}"
            VERBATIM
    )

    # Create a custom target that depends on the output file
    add_custom_target(${CUSTOM_TARGET_NAME} ALL
            DEPENDS ${OUTPUT_PATH}
    )

    set_target_properties(${CUSTOM_TARGET_NAME} PROPERTIES GENERATED_INC_FILE ${OUTPUT_PATH})

    # Add the custom target as a dependency to the given target
    add_dependencies(${TARGET_NAME} ${CUSTOM_TARGET_NAME})

    # Add OUTPUT_PATH to the global list
    get_property(EXISTING_FILES GLOBAL PROPERTY PORTAL_GENERATED_INC_FILES)
    list(APPEND EXISTING_FILES ${OUTPUT_PATH})
    set_property(GLOBAL PROPERTY PORTAL_GENERATED_INC_FILES "${EXISTING_FILES}")
endfunction()

function(create_frozen_map_target GENERATED_FILENAME)
    find_package(Python3 COMPONENTS Interpreter)
    if (NOT Python3_FOUND)
        message(FATAL_ERROR "Python 3 not found. Please install Python 3 or specify its location.")
    endif()

    # Get all generated inc files
    get_property(INC_FILES GLOBAL PROPERTY PORTAL_GENERATED_INC_FILES)

    if(NOT INC_FILES)
        message(WARNING "No generated inc files found for frozen map target")
        return()
    endif()

    get_target_property(BINARY_DIR portal-core BINARY_DIR)
    set(OUTPUT_PATH ${BINARY_DIR}/${GENERATED_FILENAME})

    # Collect all prepare_consteval_strings targets to get their output files as dependencies
    set(DEPENDENCY_TARGETS "")
    get_property(ALL_TARGETS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
    foreach(TARGET ${ALL_TARGETS})
        if(${TARGET} MATCHES "_prepare_consteval_strings$")
            list(APPEND DEPENDENCY_TARGETS ${TARGET})
        endif()
    endforeach()

    # Use add_custom_command to track file dependencies properly
    add_custom_command(
            OUTPUT ${OUTPUT_PATH}
            COMMAND ${Python3_EXECUTABLE} ${PORTAL_MAKE_FROZEN_MAP_SCRIPT} ${OUTPUT_PATH} ${INC_FILES}
            DEPENDS ${INC_FILES} ${PORTAL_MAKE_FROZEN_MAP_SCRIPT}
            COMMENT "Creating frozen map from generated inc files"
            VERBATIM
    )

    # Create a custom target that depends on the output file
    add_custom_target(portal_frozen_map ALL
            DEPENDS ${OUTPUT_PATH}
    )

    # Add dependencies to all prepare_consteval_strings targets
    foreach(TARGET ${DEPENDENCY_TARGETS})
        add_dependencies(portal_frozen_map ${TARGET})
    endforeach()

    add_dependencies(portal-core portal_frozen_map)
endfunction()