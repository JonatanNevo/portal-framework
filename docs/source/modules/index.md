# Modules

Portal Framework is organized into independent, layered modules.

---

## Dependency Graph

```{mermaid}

flowchart TD
    Engine --> Input
    Engine --> Networking
    Engine --> Serialization
    Input --> Application
    Serialization --> Core
    Networking --> Core
    Application --> Core
```

---

## Module Overview

| Module                            | Purpose          | Key Components                   |
|-----------------------------------|------------------|----------------------------------|
| [Core](core.md)                   | Foundation layer | Logging, math, jobs, memory      |
| [Application](application.md)     | App lifecycle    | Window, module system, events    |
| [Engine](engine.md)               | Game engine      | Renderer, scene graph, resources |
| [Input](input.md)                 | Input handling   | Keyboard, mouse, gamepad         |
| [Networking](networking.md)       | Network layer    | Client, server, messages         |
| [Serialization](serialization.md) | Data persistence | Binary format, type registration |

---

```{toctree}
:maxdepth: 1
:hidden:

core
application
input
engine
serialization
networking
```