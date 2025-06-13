cmake_minimum_required(VERSION 3.30)
project(portal_framework)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(core)
add_subdirectory(networking)
add_subdirectory(application)
add_subdirectory(input)
add_subdirectory(gui)
add_subdirectory(serialization)
add_subdirectory(assets)
add_subdirectory(engine)

add_executable(portal main.cpp)
target_link_libraries(portal PUBLIC portal::networking portal::gui portal::serialization portal::assets)