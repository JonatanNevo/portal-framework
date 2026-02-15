<div align="center">
  
<img src="https://raw.githubusercontent.com/JonatanNevo/portal-framework/refs/heads/main/docs/source/_static/portal_splash_dark.png" width="400"/>

# Portal Framework
**A modular cross-platform C++ framework for creating 3D graphics applications using C++23 and Vulkan**

[![License](https://img.shields.io/github/license/JonatanNevo/portal-framework)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-blue)](https://github.com/JonatanNevo/portal-framework)
[![C++](https://img.shields.io/badge/C++-23-blue)](https://github.com/JonatanNevo/portal-framework)
[![Vulkan](https://img.shields.io/badge/renderer-Vulkan%201.4-red)](https://www.vulkan.org/)
[![Build](https://img.shields.io/github/actions/workflow/status/JonatanNevo/portal-framework/build-multi-platform.yml)](https://github.com/JonatanNevo/portal-framework/actions/workflows/build-multi-platform.yml)

[Documentation](https://jonatannevo.github.io/portal-framework/)

</div>

<div align="center">
  <img width="1200" alt="image" src="https://raw.githubusercontent.com/JonatanNevo/portal-framework/refs/heads/main/docs/source/_static/engine_test_preview.png" />
</div>

## About
### Portal Framework
Portal Framework is a modular, cross-platform, C++ Framework for creating 3D applications.  
The Framework consists of independent modules that are built on top of vcpkg for package management.  
Each module can be used independently, and are built, versioned, and packaged independently.

### Portal Engine
Portal Engine is a game engine built on top of the Portal Framework.  
I made this engine to learn about, and experiment with game engine development and architecture.  
The focus so far was on a establishing  stable infrastructure to help with the development of future features.

**[Read More About The Framework](https://jonatannevo.github.io/portal-framework/introduction/index.html)**

> [!IMPORTANT]  
> Note that `Portal Framework` is still in its early stages of development.  
> Everything is subject to change.  
> To get more information regarding the state of the project, see [The Docs](https://jonatannevo.github.io/portal-framework/development-status.html)

## Platform Support

- **Windows 11** - Clang 20+ (recommended) or MSVC 17+
- **macOS 15+** (Sequoia or later)
- **Linux** - Clang 20+ or GCC 14+

## Getting Started

### Quick Start with Portal Tool

The `portal-tool` is the recommended way to create new Portal Framework projects.

**Requirements:** Python 3.12+, CMake 3.30+, Ninja, Git, C++23 compiler

```bash
pip install portal-tool
portal-tool install    # Install platform dependencies
portal-tool init       # Create a new project
```
To run the editor use:
```bash
portal-engine editor # Comes installed with `portal-tool`
```
Or run the runtime
```bash
portal-engine runtime
```

> **Note:** Currently `portal-tool` is only tested on Windows.

**[Full Installation Guide](https://jonatannevo.github.io/portal-framework/getting-started/installation.html)**

### Building from Source

```bash
cmake --preset ninja-multi
cmake --build --preset debug
```

**[Complete Build Instructions](https://jonatannevo.github.io/portal-framework/contributing/building.html)**

## Contributing

Contributions are welcome! Please see the **[Contributing Guide](https://jonatannevo.github.io/portal-framework/contributing/)** for:

- Code style guidelines
- Build and test instructions
- Documentation standards
- Pull request process

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Generative AI Usage

As a personal project and a learning experience, I have limited the usage of generative AI for the bulk of the programming.

Generative AI was used in the following areas:
- Writing testing boilerplate code
- Baseline for API documentation
- Debugging assistance


## Related Repositories

- [Portal Tool](https://github.com/JonatanNevo/portal-tool) – The repository for the `portal-tool` cli.
- [Portal Vcpkg Registry](https://github.com/JonatanNevo/portal-vcpkg-registry) – The vcpkg registry for Portal Framework.
