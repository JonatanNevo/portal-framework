function(portal_add_test_target TARGET_NAME)
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs SOURCES LIBRARIES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT Catch2_FOUND)
        find_package(Catch2 CONFIG REQUIRED)
    endif()

    enable_testing()

    if(NOT ARG_SOURCES)
        file(GLOB_RECURSE TEST_SOURCES "*tests.cpp")
        file(GLOB_RECURSE TEST_HEADERS "*.h")
        set(ARG_SOURCES ${TEST_SOURCES} ${TEST_HEADERS})
    endif()

    set(TEST_TARGET ${TARGET_NAME}-test)
    add_executable(${TEST_TARGET} ${ARG_SOURCES})

    target_link_libraries(${TEST_TARGET}
            PRIVATE
            ${TARGET_NAME}
            Catch2::Catch2
            Catch2::Catch2WithMain
            ${ARG_LIBRARIES}
    )

    target_compile_definitions(${TARGET_NAME} PUBLIC PORTAL_TEST)

    include(Catch)
    catch_discover_tests(${TEST_TARGET})
endfunction()

function(portal_build_tests FOLDER_NAME)
    if (PORTAL_BUILD_TESTS)
        enable_testing()
        add_subdirectory(${FOLDER_NAME})
    endif ()
    unset(PORTAL_BUILD_TESTS CACHE)
endfunction()