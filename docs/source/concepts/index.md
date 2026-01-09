# Concepts

```{Warning}
This section is still under construction, everything is subject to change and might be out of date!
```

Deep-dive explanations of Portal Engine's design and architecture.

These articles explain the *why* behind design decisions. Read them to build a mental model of how the engine works.

---

## Architecture & Design

Understanding how Portal Engine is structured.

- [Engine Architecture](engine-architecture.md) — Module layering, dependencies, plugin system
- [Frame Lifecycle](frame-lifecycle.md) — What happens each frame, update vs render

## Systems

How the major systems work internally.

- [Job System](job-system.md) — Job system, coroutines, what runs where
- [Resource Management](resource-management.md) — Ownership, lifetimes, GPU resources


## BuildSystem

Overview of the build system, both for the Framework itself and for games built on it.

- [BuildSystem Overview](buildsystem.md)


---

## Related

- [Modules](../modules/index.md) — Quick reference for each module
- [API Reference](../api/index.md) — Detailed API documentation
- [Contributing](../contributing/index.md) — Guidelines for contributors

---

```{toctree}
:maxdepth: 1
:hidden:

engine-architecture
frame-lifecycle
job-system
resource-management
buildsystem
```