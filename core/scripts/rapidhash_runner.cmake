
add_executable(rapidhash_runner ${CMAKE_CURRENT_LIST_DIR}/main.cpp)
target_sources(rapidhash_runner PRIVATE
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}
        FILES ${CMAKE_CURRENT_LIST_DIR}/uint128_t.h
)
target_compile_definitions(rapidhash_runner PRIVATE PORTAL_STANDALONE)