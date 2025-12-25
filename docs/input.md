# Portal Input

The Portal Input module provides cross-platform keyboard and mouse input abstraction for building interactive applications and games.

## Features

- [InputManager](../input/portal/input/input_manager.h) - Central input hub managing keyboard and mouse state
- [Input Types](../input/portal/input/input_types.h) - Unified Key enum, KeyState, Axis, CursorMode, and modifier flags
- [Input Events](../input/portal/input/input_events.h) - Event-driven input handling with KeyPressed, KeyReleased, KeyRepeat, MouseMoved, MouseScrolled events
- [InputEventConsumer](../input/portal/input/input_event_consumer.h) - Platform abstraction interface for receiving input from window systems

## API Documentation

Comprehensive API documentation with usage examples is available via Doxygen. All input module header files now include detailed documentation covering:

- Event-driven vs polling input patterns
- Key state lifecycle (Released -> Pressed -> Repeat)
- ECS integration via InputComponent
- Cursor control and modifier keys
- Thread safety and performance characteristics

Build the documentation with:

```bash
cmake --preset <your-preset> -DPORTAL_BUILD_DOCS=ON
cmake --build --preset <your-build-preset> --target doxygen
```

Generated HTML documentation will be in `docs/api/html/index.html`.

## Architecture

The input system supports two complementary approaches to input handling:

### Event-Driven Pattern
Best for UI interactions, one-shot actions (pause menu, screenshot capture), and keyboard shortcuts. Register event handlers to respond to input changes as they occur.

### Polling Pattern
Best for continuous actions (player movement, camera control) and gameplay logic that runs in sync with the game loop. Query input state each frame during system updates.

### Platform Abstraction
The InputManager abstracts platform-specific input APIs (GLFW, native Win32, etc.) into Portal's unified Key enum. Game code queries input state and responds to events without any platform-specific code.

### Key State Lifecycle
Each key progresses through a well-defined state lifecycle:
- **Released** - Key is up (default state)
- **Pressed** - Key just went down THIS frame (fires KeyPressedEvent)
- **Repeat** - Key is being held down (transitioned from Pressed after one frame)

The transition from Pressed to Repeat happens automatically each frame, enabling both "currently down" checks and "initial press only" detection.

### ECS Integration
Entities access input through the InputComponent, which holds a pointer to the InputManager. The SystemOrchestrator automatically patches InputComponent instances during scene loading, allowing systems to query input state without coupling to the global InputManager.

See the Doxygen-generated documentation for detailed usage examples, code samples, and complete API reference.
