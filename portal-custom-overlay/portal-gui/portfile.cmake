set(SOURCE_PATH "C:\\Code\\portal-framework\\gui")

# for the future
#vcpkg_from_github(
#        OUT_SOURCE_PATH SOURCE_PATH
#        REPO Microsoft/vcpkg-docs
#        REF "${VERSION}"
#        SHA512 0  # This is a temporary value. We will modify this value in the next section.
#        HEAD_REF cmake-sample-lib
#)

vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
        OPTIONS
        -DPORTAL_FIND_PACAKGE=ON
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(
        PACKAGE_NAME portal-gui
        CONFIG_PATH share/portal-gui
)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

configure_file("${CMAKE_CURRENT_LIST_DIR}/usage" "${CURRENT_PACKAGES_DIR}/share/${PORT}/usage" COPYONLY)

vcpkg_copy_pdbs()