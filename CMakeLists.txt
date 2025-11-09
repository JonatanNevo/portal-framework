cmake_minimum_required(VERSION 3.30)

project(portal_framework)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(llvm-adt)
add_subdirectory(core)
add_subdirectory(networking)
add_subdirectory(application)
add_subdirectory(input)
add_subdirectory(serialization)
add_subdirectory(engine)

create_frozen_map_target(generated/portal/core/strings/frozen_mapping.h)
