# Portal Framework

A modular cross-platform framework for creating C++ applications, with focus on graphics (3D) application.

## Supported Platforms
> MSVC builds would have a slower start time due to my string hash implementation not being constexpr on MSVC
- Windows 11
  - Clang 19 or later [recommended]
  - MSVC 17 or later
- macOS 15 (Sequoia) or later
> Linux build still have some issues with vcpkg
- Linux
  - Clang 19 or later
  - GCC 15 or later 

# Installation
TODO
## vcpkg (recommended)
TODO
## Submodule
TODO

# Standalone Build
## xcode
```shell
cmake --preset xcode
```
This will generate an Xcode project in the `build/xcode` directory. You can open the project in Xcode and build it from there.
## visual studio
```bash
cmake --preset vs2022
```
This will generate a Visual Studio solution in the `build/vs2022` directory.
> vs2026 is also supported, but cmake 4.2+ is required



# Modules
 - [**Portal Core**](docs/core.md) - The core module, provides common functionalities and libraries 
 - [**Portal Serialization**](docs/serialization.md) - Provides serialization (ordered) and archiving (named) for C++ objects to various formats
 - [**Portal Networking**](docs/networking.md) - Provides networking capabilities for C++ applications (aimed at game networking, lacks http)
 - [**Portal Application**](docs/application.md) - Provides a base application class for creating C++ applications (non graphical applications)
 - [**Portal GUI**](docs/gui.md) - Provides a GUI framework for creating graphical applications (based on ImGui)
 - [**Portal Input**](docs/input.md) - Provides input handling for C++ applications (keyboard, mouse, gamepad)

# Architecture

Detailed architecture documentation is available in the `docs/architecture/` directory:

- [**Core Systems Architecture**](docs/architecture/core-systems.md) - In-depth documentation of the job system, memory allocators, concurrency primitives, and string ID system

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
