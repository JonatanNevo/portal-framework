# Workaround for slang/dbus RPATH cycle: shader-slang is release-only while nativefiledialog-extended
# (nfd::nfd) embeds an absolute path to libdbus-1.so per-config — debug variant in Debug builds.
# We rewrite nfd::nfd's INTERFACE_LINK_LIBRARIES to always use the release libdbus.
# Delete this file when shader-slang ships debug exports or nfd stops hardcoding the absolute path.

function(_portal_fix_vcpkg_nfd_dbus_path)
    if (NOT LINUX OR NOT DEFINED VCPKG_INSTALLED_DIR)
        return()
    endif ()

    if (NOT TARGET nfd::nfd)
        return()
    endif ()

    set(_release_dbus "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/libdbus-1.so")
    if (NOT EXISTS "${_release_dbus}")
        return()
    endif ()

    set_property(TARGET nfd::nfd PROPERTY INTERFACE_LINK_LIBRARIES "${_release_dbus}")

    get_property(_logged GLOBAL PROPERTY PORTAL_VCPKG_NFD_DBUS_FIX_LOGGED)
    if (NOT _logged)
        message(STATUS "Pinned nfd::nfd libdbus link to release lib dir to break slang/dbus RPATH cycle")
        set_property(GLOBAL PROPERTY PORTAL_VCPKG_NFD_DBUS_FIX_LOGGED TRUE)
    endif ()
endfunction()