# Portal Framework

A modular cross-platform framework for creating C++ applications, with focus on graphics (3D) application.

## Supported Platforms
- Windows
  - Clang 19 (or clang-cl) or later [recommended]
  - MSVC 19.44 or later
- Linux
- macOS

# Installation
TODO
## vcpkg (recommended)
TODO
## Submodule
TODO
## xcode
TODO
```shell
cmake --preset xcode
```
This will generate an Xcode project in the `build/xcode` directory. You can open the project in Xcode and build it from there.
## visual studio 
```
cmake --preset vs2022
```

# Modules
 - [**Portal Core**](docs/core.md) - The core module, provides common functionalities and libraries 
 - [**Portal Serialization**](docs/serialization.md) - Provides serialization (ordered) and archiving (named) for C++ objects to various formats
 - [**Portal Networking**](docs/networking.md) - Provides networking capabilities for C++ applications (aimed at game networking, lacks http)
 - [**Portal Application**](docs/application.md) - Provides a base application class for creating C++ applications (non graphical applications)
 - [**Portal GUI**](docs/gui.md) - Provides a GUI framework for creating graphical applications (based on ImGui)
 - [**Portal Input**](docs/input.md) - Provides input handling for C++ applications (keyboard, mouse, gamepad)

# Architecture

# Examples

# Roadmap
## Missing modules
* audio system
* animations
* physics engine
* scripting capabilities
* networking
* mods (based on web assembly)

# License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
