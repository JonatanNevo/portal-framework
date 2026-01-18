find_program(UV_EXECUTABLE NAMES uv)

get_property(DOCS_TARGET GLOBAL PROPERTY PORTAL_DOCS_TARGETS)

add_custom_target(docs
        DEPENDS ${DOCS_TARGET}
)