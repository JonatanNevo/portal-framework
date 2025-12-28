# ![Portal Framework](docs/source/_static/portal_splash_dark.png)

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

# Usage
## Portal Tool (Recommended)
> Currently `portal-tool` only works on Windows, linux has known issues and macos is not yet tested
### Requirements
- Python 3.12 or later
- CMake 3.30 or later
- Ninja build system
- Git
- C++ 23+ compiler

### Installation
Install the portal tool package
```shell
pip install portal-tool
```

Install the platform specific dependencies
```shell
portal-tool install
```

Initialize a project using portal engine
```shell
portal-tool init
```

Build the new project
```shell
cd <project-name>
cmake --preset ninja-multi
cmake --build --preset debug/development/release
```

# Development

## Cmake
```shell
cmake --preset ninja-multi
```

```shell
cmake --build --preset debug/development/release
```

## External Toolchains
### xcode
```shell
cmake --preset xcode
```
This will generate an Xcode project in the `build/xcode` directory. You can open the project in Xcode and build it from there.
### visual studio
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
TODO

# Roadmap
## Missing modules
* audio system
* animations
* physics engine
* scripting capabilities
* networking
* mods (based on web assembly)

# Generative AI Usage
As a personal project and a learning experience I tried to limit the usage of generative AI for the bulk of the programming.

Generative AI was used in the following places:
- Writing testing boilerplate code
- Baseline for API documentation
- Debugging

# License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
