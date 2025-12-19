# API Documentation build configuration

# Require Doxygen for documentation generation
find_package(Doxygen REQUIRED)

if(NOT DOXYGEN_FOUND)
    message(FATAL_ERROR "Doxygen is required to build API documentation. Please install Doxygen.")
endif()

# Read project version from core/version.txt
file(READ "${CMAKE_SOURCE_DIR}/core/version.txt" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)

# Configure Doxyfile with CMake variables
set(DOXYGEN_IN ${CMAKE_SOURCE_DIR}/Doxyfile.in)
set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

# Custom target to generate documentation
add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
)

message(STATUS "API documentation build target 'docs' configured")
message(STATUS "  - Build with: cmake --build . --target docs")
message(STATUS "  - Output: ${CMAKE_SOURCE_DIR}/docs/api/html/index.html")