cmake_minimum_required(VERSION 3.30)
project(engine_test)

#find_package(portal-engine CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 20)

portal_add_game(engine_test
        SOURCES source/main.cpp
        DISPLAY_NAME "Engine Test"
)

portal_game_configure_installer(
        engine_test
        DESCRIPTION "Engine Test Example Project"
        VENDOR "Jonatan Nevo"
        CONTACT "jonatannevo-git@proton.me"
)
