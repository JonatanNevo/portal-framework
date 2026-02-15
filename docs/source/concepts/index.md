# Concepts

```{Warning}
This section is still under construction. Articles are being actively updated and some content may be incomplete or out of date.
```

Deep-dive explanations of Portal Engine's design, architecture, and the philosophy behind its development.  
These articles explain the *why* behind design decisions to help you build a mental model of how the engine works.

---

## Architecture & Design

Fundamental principles of the engine's structure and execution flow.

````{grid} 2
:gutter: 3

```{grid-item-card} Engine Architecture 🏗️
:link: engine-architecture
:link-type: doc

Overview of module layering, dependency rules, and how the module system enables decoupled functionality.
```

```{grid-item-card} Frame Lifecycle 🔄
:link: frame-lifecycle
:link-type: doc

A walkthrough of what happens each frame, including the separation of update and render phases.
```
````

---

## Core Systems

Deep dives into the internal workings of major engine systems.

````{grid} 2
:gutter: 3

```{grid-item-card} Job System 🧵
:link: job-system
:link-type: doc

Architecture of the task-based parallelism system, including coroutines and thread safety guarantees.
```

```{grid-item-card} Resource Management 📦
:link: resource-management
:link-type: doc

How assets are identified, loaded, streamed, and managed across CPU and GPU memory.
```
````

---

## Infrastructure

The tools and systems that support the development of the framework and games.

````{grid} 1
:gutter: 3

```{grid-item-card} Build System 🛠️
:link: buildsystem
:link-type: doc

Overview of the CMake-based build infrastructure and project management.
```
````

---

## Related Information

- [Modules](../modules/index.md) — Technical reference for each engine module
- [API Reference](../api/index.md) — Detailed documentation for types and functions
- [Contributing](../contributing/index.md) — Guidelines for engine contributors

```{toctree}
:maxdepth: 1
:hidden:

engine-architecture
frame-lifecycle
job-system
resource-management
buildsystem
```