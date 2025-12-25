# Context Architecture

The Portal Framework uses **tiered contexts** instead of global states to reduce coupling and improve testability. Each context is a lightweight struct that holds references to data and services required for a specific scope of the application.

## Benefits

- **Better unit testing**: Contexts can be easily mocked or stubbed
- **Reduced coupling**: Components only depend on the context they need
- **Clear ownership**: Explicit lifetime management through references
- **No global state**: Easier to reason about initialization order

## Context Hierarchy

```
EngineContext (Application lifetime)
    ├─ Renderer
    ├─ ResourceRegistry
    ├─ Window
    ├─ Input
    │
    └─→ RendererContext (Rendering system lifetime)
            ├─ VulkanContext (GPU lifetime)
            │   ├─ Instance
            │   ├─ PhysicalDevice
            │   └─ LogicalDevice
            ├─ RenderTarget
            └─ GlobalDescriptorSetLayout
                │
                └─→ DrawContext (Per-frame lifetime)
                        └─ RenderObjects[]
```

## Context Descriptions

### EngineContext
**Lifetime**: Application  
**Instance Count**: Singleton  
**Location**: `portal/engine/engine_context.h`

The top-level context holding the highest level of abstractions. Contains:
- **Renderer**: The rendering system interface
- **ResourceRegistry**: Resource management
- **Window**: Window system and surface management
- **Input**: Input handling (keyboard, mouse, gamepad)

This context is passed to application layers and modules that need access to core engine systems.

### RendererContext
**Lifetime**: Rendering system initialization to shutdown
**Instance Count**: One per renderer  
**Location**: `portal/engine/renderer/renderer_context.h`

Holds renderer-specific data and configuration. Contains:
- **VulkanContext**: GPU device and instance management
- **RenderTarget**: Output framebuffer/swapchain
- **GlobalDescriptorSetLayout**: Shared descriptor layouts for materials/shaders

Used by rendering code that needs access to GPU resources and render targets.

### DrawContext
**Lifetime**: Per-frame (recreated each frame)  
**Instance Count**: One per frame  
**Location**: `portal/engine/scene/frame_context.h`

Contains all data needed to render a single frame. Contains:
- **RenderObjects**: List of visible objects to render with their transforms, materials, and geometry

This context is built by the scene graph during culling and consumed by the renderer.

### VulkanContext  
**Lifetime**: GPU initialization to shutdown  
**Instance Count**: Singleton  
**Location**: `portal/engine/renderer/vulkan/vulkan_context.h`

Manages the Vulkan graphics API state. Contains:
- **Instance**: Vulkan instance and validation layers
- **PhysicalDevice**: Selected GPU device
- **LogicalDevice**: Vulkan logical device and queue families

This is the lowest-level graphics context, wrapping the Vulkan API.

## Usage Example

```cpp
// Application creates top-level context
EngineContext engine_ctx(renderer, registry, window, input);

// Renderer creates its own context
RendererContext renderer_ctx(vulkan_ctx, render_target, descriptor_layouts);

// Each frame, build draw context
DrawContext draw_ctx;
scene.cull_and_populate(draw_ctx, camera);

// Pass context down to rendering
renderer.render(renderer_ctx, draw_ctx);
```

## Best Practices

### Passing Contexts
- Pass contexts by **reference**, never by value
- Use `const` references when read-only access is sufficient
- Prefer passing the most specific context needed (don't pass `EngineContext` if `RendererContext` suffices)

### Storing Contexts
- Store as `std::reference_wrapper<T>` or as reference when the context is not owned
- Never store raw pointers to contexts
- Contexts should not own each other (use references)

### Context Lifetime
- Ensure referenced objects outlive the context
- Create contexts on the stack when possible
- Parent contexts should be created before child contexts
- Destroy in reverse order of creation

### Testing
- Create minimal contexts with mock implementations for unit tests
- Only populate the specific services needed for each test
- Use test fixtures to manage context lifecycle

## Design Rationale

Traditional game engines often use singletons or global state for core systems like rendering and resources. This creates several problems:

1. **Hard to test**: Can't easily swap implementations or mock dependencies
2. **Hidden dependencies**: Unclear what each component actually needs
3. **Thread safety**: Global mutable state requires complex synchronization
4. **Initialization order**: Singletons have undefined initialization order

The context pattern solves these issues by:
- Making dependencies **explicit** through constructor parameters
- Enabling **dependency injection** for testing
- Providing **clear lifetime semantics** (contexts are RAII types)
- Allowing **multiple instances** when needed (e.g., multiple windows)