# Introduction

---

## What Is Portal Framework?

Portal Framework is a modular, cross-platform, C++ Framework for creating **3D** applications.  
The Framework consists of independent modules that are built on top of vcpkg for package management.    
Each module can be used independently, and are built, versioned, and packaged independently.  

Currently, there are six modules:

* [Core](../modules/core.md) – The foundation of all other modules, contains common, general-purpose features, such as
  logging, utilities, etc...
* [Serialization](../modules/serialization.md) – Archiving and serialization support.
* [Application](../modules/application.md) – Application lifecycle, module system, and settings management.
* [Input](../modules/input.md) – Input abstraction for keyboard, mouse, and gamepad.
* [Networking](../modules/networking.md) - Client-Server infrastructure, with a focus on real-time communication.
* [Engine](../modules/engine.md) – A Game engine implemented on top of the Portal Framework.

---

## What Is Portal Engine?

Portal Engine is a game engine built on top of the Portal Framework.
A big focus of the engine is to use and experiment with the latest technologies, such as **C++ 23**, **Slang Shader
language**, **Vulkan 1.4** and more.

Learn more about the [Engine Architecture](architecture.md) and [Engine Features](features.md).

---

## Why Create An Engine From Scratch?
Long story short: I wanted to learn more about game engines' architecture.  
After studying various open source engines and examples, I identified opportunities to explore different design approaches and architectural decisions.  
Building from scratch provides the freedom to experiment with modern technologies and establish patterns that align with my development philosophy.

### What Quantifies As "From Scratch"?
I tried to implement as much as I could myself.  
Meaning writing the vulkan renderer from scratch, the resource management, the job system, etc...  
  
I did look and took inspiration from other engines and open-source libraries, such as [Hazel](https://github.com/StudioCherno), [Godot](https://godotengine.org/), [Unreal](https://www.unrealengine.com/en-US/ue-on-github) and [Ogre3D](https://www.ogre3d.org/).    
Additionally, A main resource I used as a reference was the book [Game Engine Architecture](https://www.gameenginebook.com/) by Jason Gregory.  
As well as other well-known books like [Real Time Rendering](https://www.realtimerendering.com/), [Ray Tracing In One Weekend](https://raytracing.github.io/) and [Physically Based Rendering From Theory To Implementation](https://www.pbr-book.org/).  
  
I must add that while this seems like a comprehensive list, in practice I managed to implement only a small part of the features I have planned.

### Generative AI Usage
As a personal project and a learning experience, I have limited the usage of generative AI for the bulk of the programming.

Generative AI was used in the following areas:
- Writing testing boilerplate code
- Baseline for API documentation
- Debugging assistance

---

## Current State Of The Project

Portal Framework is in the **early** stages of development.  
The focus so far was on establishing a stable infrastructure to help with the development of future features.  
As a result, most of the features so far are less "visual" and more "functional"  
  
Most of the required infrastructure is now in place, such as:

* cross-platform build system
* application layer
* Entity Component System
* resource management
* and more...

Thanks to that, the focus is now shifting towards more "traditional" game engine features.    
You can see the upcoming roadmap and progress [here](../development-status.md)

---

## Quick Links

- Ready to dive in? → [Getting Started](../getting-started/index.md)
- Have a specific question? → [How-To](../how-to/index.md)
- Detailed descriptions of concepts introduced in this section → [Concepts](../concepts/index.md)
- Looking for API docs? → [API Reference](../api/index.md)

```{toctree}
:maxdepth: 1
:hidden:

architecture
features
```