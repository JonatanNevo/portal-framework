cmake_minimum_required(VERSION 3.30)

project(llvm-adt VERSION 0.0.1 DESCRIPTION "The ADT containers from the llvm project" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# On z/OS, threads cannot be used because TLS is not supported.
if (CMAKE_SYSTEM_NAME MATCHES "OS390")
    option(LLVM_ENABLE_THREADS "Use threads if available." OFF)
else()
    option(LLVM_ENABLE_THREADS "Use threads if available." ON)
endif()

if(WIN32)
    set(HAVE_MACH_MACH_H 0)
    set(HAVE_MALLOC_MALLOC_H 0)
    set(HAVE_PTHREAD_H 0)
    set(HAVE_SYS_MMAN_H 0)
    set(HAVE_SYSEXITS_H 0)
    set(HAVE_UNISTD_H 0)
    set(HAVE_SYS_IOCTL_H 0)
    set(LLVM_ON_UNIX 0)
elseif(APPLE)
    set(HAVE_MACH_MACH_H 1)
    set(HAVE_MALLOC_MALLOC_H 1)
    set(HAVE_PTHREAD_H 1)
    set(HAVE_SYS_MMAN_H 1)
    set(HAVE_SYSEXITS_H 1)
    set(HAVE_UNISTD_H 1)
    set(HAVE_SYS_IOCTL_H 1)
    set(LLVM_ON_UNIX 1)
elseif(UNIX)
    set(HAVE_MACH_MACH_H 0)
    set(HAVE_MALLOC_MALLOC_H 0)
    set(HAVE_PTHREAD_H 1)
    set(HAVE_SYS_MMAN_H 1)
    set(HAVE_SYSEXITS_H 1)
    set(HAVE_UNISTD_H 1)
    set(HAVE_SYS_IOCTL_H 1)
    set(LLVM_ON_UNIX 1)
else()
    set(HAVE_MACH_MACH_H 0)
    set(HAVE_MALLOC_MALLOC_H 0)
    set(HAVE_PTHREAD_H 0)
    set(HAVE_SYS_MMAN_H 0)
    set(HAVE_SYSEXITS_H 0)
    set(HAVE_UNISTD_H 0)
    set(HAVE_SYS_IOCTL_H 0)
    set(LLVM_ON_UNIX 0)
endif()

configure_file("llvm/include/llvm/Config/llvm-config.h.cmake" "llvm/include/llvm/Config/llvm-config.h")

file(GLOB_RECURSE LLVM_SOURCES CONFIGURE_DEPENDS "llvm/lib/*.cpp" "llvm/lib/*.c")
file(GLOB_RECURSE LLVM_HEADER CONFIGURE_DEPENDS "llvm/include/*.h")

add_library(llvm-adt STATIC ${LLVM_SOURCES})
add_library(llvm::adt ALIAS llvm-adt)
target_link_libraries(llvm-adt PRIVATE ntdll)

target_sources(llvm-adt PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/llvm/include
        FILES ${LLVM_HEADER}
)

target_include_directories(llvm-adt SYSTEM PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/llvm/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/llvm/include>
        $<INSTALL_INTERFACE:include>
)

install(TARGETS llvm-adt
        EXPORT llvm-adt-targets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        FILE_SET HEADERS DESTINATION include
)

# Generate and install the export targets
install(EXPORT llvm-adt-targets
        FILE llvm-adt-targets.cmake
        NAMESPACE llvm::
        DESTINATION share/llvm-adt
)

# Create and install the config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/llvm-adt-config-version.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)
unset(PROJECT_VERSION)

configure_package_config_file(
        "../cmake/llvm-adt-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/llvm-adt-config.cmake"
        INSTALL_DESTINATION share/llvm-adt
)

# Install the config files
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/llvm-adt-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/llvm-adt-config-version.cmake"
        DESTINATION share/llvm-adt
)