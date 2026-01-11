![Portal Framework](docs/source/_static/portal_splash_dark.png)

# Portal Framework

A modular cross-platform C++ framework for creating 3D graphics applications using C++23 and Vulkan.

> [!IMPORTANT]  
> Note that `Portal Framework` is still in its early stages of development.  
> Everything is subject to change.  
> To get more information regarding the state of the project, see [The Docs](https://jonatannevo.github.io/portal-framework/development-status.html)

**[Full Documentation](https://jonatannevo.github.io/portal-framework/)**

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

> **Note:** Currently `portal-tool` works on Windows only. Linux and macOS are untested.

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
