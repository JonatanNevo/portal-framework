cmake_minimum_required(VERSION 3.30)
project(shader_playground VERSION 1.0.0)

#find_package(portal-engine CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 20)

portal_add_game(shader_playground
        SOURCES source/main.cpp
        VENDOR "Jonatan Nevo"
        CONTACT "jonatannevo-git@proton.me"
)

# Uncomment this if you have Qt Installer Framework installed and you want to package an installer
# portal_game_configure_installer(
#         shader_playground
# )
