cmake_minimum_required(VERSION 3.30)
project(engine_test VERSION 1.0.0)

#find_package(portal-engine CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 20)

portal_add_game(engine_test
        SOURCES source/main.cpp
        DISPLAY_NAME "Engine Test"
)

# Uncomment this if you have Qt Installer Framework installed and you want to package an installer
# portal_game_configure_installer(
#         engine_test
#         DESCRIPTION "Engine Test Example Project"
#         VENDOR "Jonatan Nevo"
#         CONTACT "jonatannevo-git@proton.me"
# )
