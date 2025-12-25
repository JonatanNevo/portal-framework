# Portal Application

The Portal Application module provides the core application framework and module system for building Portal applications.

## Features

- [Application](../application/portal/application/application.h) - Main application class with game loop and lifecycle management
- [Module System](../application/portal/application/modules/module.h) - Compile-time dependency injection with tag-based lifecycle hooks
  - [BaseModule](../application/portal/application/modules/base_module.h) - Polymorphic base for all modules
  - [TaggedModule](../application/portal/application/modules/module.h) - CRTP template for dependency injection
  - [ModuleStack](../application/portal/application/modules/module_stack.h) - DI container with topological sorting
  - [ModuleTags](../application/portal/application/modules/tags.h) - Lifecycle phase declarations
- [FrameContext](../application/portal/application/frame_context.h) - Per-frame data passed through lifecycle hooks
- [Entry Point](../application/portal/application/entry_point.h) - Platform-specific main entry point macro

## API Documentation

Comprehensive API documentation is available via Doxygen. Build the documentation with:

```bash
cmake --preset <your-preset> -DPORTAL_BUILD_DOCS=ON
cmake --build --preset <your-build-preset> --target doxygen
```

Generated HTML documentation will be in `docs/api/html/index.html`.

## Architecture

The module system implements compile-time dependency injection with zero runtime overhead. Modules declare:
- **Dependencies** - Other modules they require (resolved at registration time)
- **Lifecycle Tags** - Which frame phases they participate in (Update, FrameLifecycle, GuiUpdate, PostUpdate, Event)

The ModuleStack performs topological sorting to ensure dependencies execute before dependents. Tag-based execution decouples module identity from execution order, allowing modules to declare when they run rather than inheriting from specific base classes.

See the Doxygen-generated documentation for detailed usage examples and API reference.