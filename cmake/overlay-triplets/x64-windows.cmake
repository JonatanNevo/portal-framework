set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

# Override to dynamic for packages that don't support static and their dependencies
if(PORT MATCHES "gamenetworkingsockets|mimalloc|shader-slang|protobuf|abseil|utf8-range|openssl")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()
