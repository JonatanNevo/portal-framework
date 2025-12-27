```{image} _static/portal_splash_dark.png
    :align: center
    :class: only-dark
```

```{image} _static/portal_splash_light.png
    :align: center
    :class: only-light
```

Welcome to the **Portal Framework** - a modular C++23 game engine framework designed for high-performance, data-oriented game development.

The Portal Framework provides a comprehensive set of systems for building modern games and interactive applications:

* **Core Systems**: Memory management, job scheduling, string handling, and concurrency primitives
* **Entity Component System (ECS)**: High-performance, cache-friendly entity management
* **Rendering**: Modern graphics pipeline with support for various rendering backends
* **Input**: Cross-platform input handling for keyboard, mouse, and gamepad
* **Networking**: Robust networking layer for multiplayer games
* **Serialization**: Efficient data serialization and deserialization

```{toctree}
:maxdepth: 1
:caption: Architecture

architecture/core-systems
```

```{toctree}
:maxdepth: 1
:caption: Modules

modules/core
modules/application
modules/input
modules/serialization
modules/networking
modules/gui
modules/context
```

```{toctree}
:maxdepth: 1
:caption: API Reference

api/core/root.rst
api/application/root.rst
api/input/root.rst
api/serialization/root.rst
api/engine/root.rst
api/networking/root.rst

```

## Getting Started

To get started with the Portal Framework, check out the [Core Systems Architecture](architecture/core-systems.md) documentation
for an overview of the framework's architecture and design principles.

## Indices and Tables

* {ref}`genindex`
* {ref}`search`