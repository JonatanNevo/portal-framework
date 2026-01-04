# Portal Engine

The Portal Engine module provides the core rendering and scene management systems for building 3D graphics applications and games.

## Features

- **Vulkan Renderer** - Modern Vulkan-based rendering pipeline with RAII wrappers and builder patterns
- **Resource Management** - Comprehensive resource loading and lifetime management system
- **Entity Component System** - High-performance ECS for scene graph management
- **Scene System** - Hierarchical scene representation with transform propagation

## Renderer Subsystem

The renderer subsystem provides a complete Vulkan-based graphics pipeline with:

- [Renderer](../engine/portal/engine/renderer/renderer.h) - Main rendering interface with frame pacing
- [VulkanContext](../engine/portal/engine/renderer/vulkan/vulkan_context.h) - Vulkan instance, device, and queue management
- [VulkanDevice](../engine/portal/engine/renderer/vulkan/vulkan_device.h) - Logical device abstraction with resource creation
- [DeletionQueue](../engine/portal/engine/renderer/deletion_queue.h) - Deferred resource destruction for in-flight frames
- [DescriptorAllocator](../engine/portal/engine/renderer/descriptor_allocator.h) - Pooled descriptor set allocation

### Architecture

The rendering system uses several key design patterns:

**RAII Wrappers**: All Vulkan objects are wrapped in RAII types using the `VulkanResource<T>` CRTP base class, ensuring automatic cleanup and debug naming support.

**Builder Pattern**: Complex Vulkan objects (pipelines, images, buffers) are created using fluent builder APIs (PipelineBuilder, ImageBuilder, BufferBuilder) that provide type-safe construction with sensible defaults.

**VMA Integration**: The `Allocated<T>` template class integrates Vulkan Memory Allocator for efficient GPU memory management with persistent mapping and coherent memory support.

**Frame Pacing**: The renderer implements N-frames-in-flight (default 3) with per-frame resources, synchronization primitives, and deletion queues to overlap CPU and GPU work.

**Deletion Queues**: Resources are destroyed using deferred deletion queues that execute in reverse order, respecting Vulkan's object lifetime dependencies.

### Subsystems

The renderer is organized into the following subsystems:

- **Queues** - Graphics, compute, transfer, and present queue management
- **Images & Textures** - Image creation, views, samplers, and texture loading
- **Shaders** - GLSL/HLSL compilation via shaderc with reflection support
- **Descriptors** - Descriptor set layouts, allocation, and binding
- **Materials** - Material system with shader parameter management
- **Pipelines** - Graphics and compute pipeline creation and caching
- **Render Targets** - Swapchain and offscreen render target management
- **Device Abstraction** - Physical device selection and logical device creation
- **Vulkan Utilities** - Format conversions, initialization helpers, and debug utilities

## API Documentation

Complete Doxygen API reference documentation is now available for the entire renderer subsystem. All 60+ renderer header files include detailed documentation covering:

**Documented Subsystems (13 total):**
- Queues (2 files) - Graphics, compute, transfer, and present queue management
- Images & Textures (7 files) - Image creation, views, samplers, and texture loading
- Shaders (4 files) - GLSL/HLSL compilation via shaderc with reflection support
- Descriptors (11 files) - Descriptor set layouts, allocation, and binding
- Materials (2 files) - Material system with shader parameter management
- Pipelines (4 files) - Graphics and compute pipeline creation and caching
- Render Targets (2 files) - Swapchain and offscreen render target management
- Core Renderer (2 files) - Main rendering interface with frame pacing
- Device Abstraction (2 files) - Physical device selection and logical device creation
- Surface (1 file) - Window surface management
- Vulkan Debug (1 file) - Debug messenger and validation layer utilities
- Vulkan Surface (1 file) - Platform-specific surface creation
- Vulkan Utilities (4 files) - Format conversions, initialization helpers, and debug utilities

**Documentation Coverage:**
- All public classes, structs, enums, and C++20 concepts have @brief descriptions
- Complete @param and @return documentation for all functions
- GPU memory layout context for data structures (Vertex, GPUMeshBuffers)
- Vulkan synchronization semantics documented (barriers, fences, semaphores)
- RAII patterns and builder APIs fully explained
- VMA memory management integration documented
- Frame synchronization and deletion queue semantics

Build the documentation with:

```bash
cmake --preset <your-preset> -DPORTAL_BUILD_DOCS=ON
cmake --build --preset <your-build-preset> --target doxygen
```

Generated HTML documentation will be in `docs/build/html/index.html`.

## Resource System

The resource system provides asynchronous loading and lifetime management:

- [ResourceRegistry](../engine/portal/engine/resources/resource_registry.h) - Central resource management
- [ResourceReference](../engine/portal/engine/resources/resource_reference.h) - Type-safe resource handles
- [ResourceDatabase](../engine/portal/engine/resources/database/resource_database.h) - Abstract resource storage interface
- Resource loaders for meshes, textures, materials, shaders, and scenes (GLTF support)

## ECS System

The Entity Component System provides high-performance entity management:

- [Registry](../engine/portal/engine/ecs/registry.h) - Entity and component storage
- [System](../engine/portal/engine/ecs/system.h) - System base class with ownership semantics
- [Entity](../engine/portal/engine/ecs/entity.h) - Entity handle with parent-child relationships

See the Doxygen-generated documentation for detailed usage examples, architecture diagrams, and complete API reference.
