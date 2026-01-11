set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

if (PORT MATCHES "portal")
    set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/../overload-compiler.cmake)
    set(VCPKG_LOAD_VCVARS_ENV ON) # Setting VCPKG_CHAINLOAD_TOOLCHAIN_FILE deactivates automatic vcvars setup so reenable it!
endif ()

# Override to dynamic for packages that don't support static and their dependencies
if(PORT MATCHES "gamenetworkingsockets|mimalloc|shader-slang|protobuf|abseil|utf8-range|openssl")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()
