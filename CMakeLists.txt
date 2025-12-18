cmake_minimum_required(VERSION 3.30)


#set(CMAKE_C_COMPILER "C:/Program Files/LLVM/bin/clang.exe")
#set(CMAKE_CXX_COMPILER "C:/Program Files/LLVM/bin/clang++.exe")
#set(CMAKE_RC_COMPILER "C:/Program Files/LLVM/bin/llvm-rc.exe")

project(portal_framework)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(core)
#add_subdirectory(networking)
add_subdirectory(application)
add_subdirectory(input)
add_subdirectory(serialization)
add_subdirectory(engine)
enable_testing()
