find_program(UV_EXECUTABLE NAMES uv)

add_custom_target(configure_uv
    COMMAND ${UV_EXECUTABLE} sync
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

get_property(DOCS_TARGET GLOBAL PROPERTY PORTAL_DOCS_TARGETS)

# Sphinx HTML generation target (main documentation build)
add_custom_target(docs
    COMMAND ${UV_EXECUTABLE} run python -m sphinx -b html -j auto
        ${CMAKE_CURRENT_SOURCE_DIR}/source
        ${CMAKE_CURRENT_SOURCE_DIR}/build/html
    DEPENDS ${DOCS_TARGET}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating Sphinx documentation (HTML)"
    VERBATIM
)