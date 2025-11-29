cmake_minimum_required(VERSION 3.30)

# Experimental packaging tools
set(CMAKE_EXPERIMENTAL_EXPORT_PACKAGE_DEPENDENCIES 1942b4fa-b2c5-4546-9385-83f254070067)

include(CheckIncludeFile)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckCXXSymbolExists)
include(CheckFunctionExists)
include(CheckStructHasMember)
include(CheckCCompilerFlag)
include(CheckCSourceCompiles)
include(CMakePushCheckState)

file(READ "version.txt" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(llvm-adt VERSION ${PROJECT_VERSION} DESCRIPTION "The ADT containers from the llvm project" LANGUAGES CXX)

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
    check_include_file(mach/mach.h HAVE_MACH_MACH_H)
    check_include_file(malloc/malloc.h HAVE_MALLOC_MALLOC_H)
    check_include_file(pthread.h HAVE_PTHREAD_H)
    check_include_file(sys/mman.h HAVE_SYS_MMAN_H)
    check_include_file(sysexits.h HAVE_SYSEXITS_H)
    check_include_file(unistd.h HAVE_UNISTD_H)
    check_include_file(sys/ioctl.h HAVE_SYS_IOCTL_H)
endif()

check_symbol_exists(_Unwind_Backtrace "unwind.h" HAVE__UNWIND_BACKTRACE)
check_symbol_exists(getpagesize unistd.h HAVE_GETPAGESIZE)
check_symbol_exists(sysconf unistd.h HAVE_SYSCONF)
check_symbol_exists(getrusage sys/resource.h HAVE_GETRUSAGE)
check_symbol_exists(isatty unistd.h HAVE_ISATTY)
check_symbol_exists(futimens sys/stat.h HAVE_FUTIMENS)
check_symbol_exists(futimes sys/time.h HAVE_FUTIMES)
check_symbol_exists(getauxval sys/auxv.h HAVE_GETAUXVAL)

# library checks
if(NOT WIN32)
    check_library_exists(pthread pthread_create "" HAVE_LIBPTHREAD)
    if (HAVE_LIBPTHREAD)
        check_library_exists(pthread pthread_rwlock_init "" HAVE_PTHREAD_RWLOCK_INIT)
        check_library_exists(pthread pthread_mutex_lock "" HAVE_PTHREAD_MUTEX_LOCK)
    else()
        # this could be Android
        check_library_exists(c pthread_create "" PTHREAD_IN_LIBC)
        if (PTHREAD_IN_LIBC)
            check_library_exists(c pthread_rwlock_init "" HAVE_PTHREAD_RWLOCK_INIT)
            check_library_exists(c pthread_mutex_lock "" HAVE_PTHREAD_MUTEX_LOCK)
        endif()
    endif()
    check_library_exists(dl dlopen "" HAVE_LIBDL)
    check_library_exists(rt shm_open "" HAVE_LIBRT)
endif()

configure_file("llvm/include/llvm/Config/llvm-config.h.cmake" "llvm/include/llvm/Config/llvm-config.h")

file(GLOB_RECURSE LLVM_HEADER CONFIGURE_DEPENDS "llvm/include/*.h")
file(GLOB_RECURSE LLVM_SOURCES CONFIGURE_DEPENDS "llvm/lib/*.cpp" "llvm/lib/*.c")
add_library(llvm-adt STATIC ${LLVM_SOURCES})
add_library(llvm::adt ALIAS llvm-adt)
set_target_properties(llvm-adt PROPERTIES EXPORT_NAME adt)

target_sources(llvm-adt PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/llvm/include ${CMAKE_CURRENT_BINARY_DIR}/llvm/include
        FILES ${LLVM_HEADER} "${CMAKE_CURRENT_BINARY_DIR}/llvm/include/llvm/Config/llvm-config.h"
)

set_target_properties(llvm-adt PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:llvm-adt,INTERFACE_INCLUDE_DIRECTORIES>)

if(WIN32)
    target_link_libraries(llvm-adt PRIVATE ntdll)
elseif(APPLE)
    target_compile_options(llvm-adt PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-fexperimental-library> $<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
endif()

install(
        TARGETS llvm-adt
        EXPORT llvm-adt-targets
        FILE_SET HEADERS
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/llvm-adt-version.cmake"
        COMPATIBILITY ExactVersion
)

install(
        EXPORT llvm-adt-targets
        FILE llvm-adt-config.cmake
        NAMESPACE llvm::
        DESTINATION share/llvm-adt
        EXPORT_PACKAGE_DEPENDENCIES
)

install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/llvm-adt-version.cmake"
        DESTINATION share/llvm-adt
)