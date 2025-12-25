# API Documentation build configuration using Sphinx + Breathe + Doxygen

find_package(Doxygen REQUIRED)

if(NOT DOXYGEN_FOUND)
    message(FATAL_ERROR "Doxygen is required to build API documentation. Please install Doxygen.")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

find_program(UV_EXECUTABLE NAMES uv)

# Read project version from core/version.txt
file(READ "${CMAKE_SOURCE_DIR}/core/version.txt" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)

## Doxygen XML generation target (preprocessing step for Sphinx)
add_custom_target(docs-doxygen
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/source/Doxyfile
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating Doxygen XML for Breathe"
    VERBATIM
)

add_custom_target(configure_uv
    COMMAND ${UV_EXECUTABLE} sync
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Sphinx HTML generation target (main documentation build)
add_custom_target(docs
    COMMAND ${UV_EXECUTABLE} run python -m sphinx -b html
        ${CMAKE_CURRENT_SOURCE_DIR}/source
        ${CMAKE_CURRENT_SOURCE_DIR}/build/html
    DEPENDS configure_uv docs-doxygen
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating Sphinx documentation (HTML)"
    VERBATIM
)