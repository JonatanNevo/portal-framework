set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_ARCHITECTURES arm64)

# Override to dynamic for packages that don't support static and their dependencies
if(PORT MATCHES "gamenetworkingsockets|mimalloc|shader-slang|protobuf|abseil|utf8-range|openssl")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()
