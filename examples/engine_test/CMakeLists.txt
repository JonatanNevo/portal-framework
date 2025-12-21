cmake_minimum_required(VERSION 3.30)
project(engine_test)

#find_package(portal-engine CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 20)

portal_add_game(engine_test
        SOURCES main.cpp
)