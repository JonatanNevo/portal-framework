# Modules ⚠️

Portal Framework is organized into independent, layered modules.

---

## Module Overview

| Module                            | Purpose          | Key Components                            |
|-----------------------------------|------------------|-------------------------------------------|
| [Core](core.md)                   | Foundation layer | Logging, math, jobs, memory               |
| [Application](application.md)     | App lifecycle    | Window, module system, events, entrypoint |
| [Engine](engine.md)               | Game engine      | Renderer, Gameplay Framework, resources   |
| [Input](input.md)                 | Input handling   | Keyboard, mouse, gamepad                  |
| [Networking](networking.md)       | Network layer    | Client, server, messages                  |
| [Serialization](serialization.md) | Data persistence | Binary format, type registration          |

---
## Module Dependency Graph

```{mermaid}

flowchart TD
    Engine --> Input
    Engine --> Networking
    Engine --> Serialization
    Input --> Application
    Serialization --> Core
    Application --> Core
    Application --> Serialization
    Networking --> Core
```

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