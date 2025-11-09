# Set the script path once when this file is included
set(PORTAL_PREPARE_CONSTEVAL_STRINGS_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/prepare_consteval_strings.py" CACHE STRING "")

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

    # Create a unique custom target name
    set(CUSTOM_TARGET_NAME "${TARGET_NAME}_prepare_consteval_strings")

    # Add custom target that runs the Python script
    add_custom_target(${CUSTOM_TARGET_NAME}
            COMMAND ${Python3_EXECUTABLE} ${SCRIPT_PATH} ${BASE_DIR} ${OUTPUT_PATH} $<TARGET_FILE:rapidhash_runner>
            COMMENT "Preparing consteval strings from ${BASE_DIR} to ${OUTPUT_PATH}"
            VERBATIM
    )
    add_dependencies(${CUSTOM_TARGET_NAME} rapidhash_runner)

    # Add the custom target as a dependency to the given target
    add_dependencies(${TARGET_NAME} ${CUSTOM_TARGET_NAME})
endfunction()