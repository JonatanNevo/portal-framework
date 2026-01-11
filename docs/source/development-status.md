# Development Status

Portal Framework is still in early development.

## High Level Roadmap

```{mermaid}
---
zoom: true
align: center
---
timeline
    MVP : Build Infrastructure
        : Engine Infrastructure
        : Resource Management
        : ECS
        : Basic Renderer
        : Serialization
        : Documentation
        : Packaging System
        : Cross Platform Support
    
    Working Game : Basic Editor
                 : Animation System
                 : Audio System
                 : Advanced Shaders
                 : Basic Networking
                 : UI System
                 : Scripting
                 : AI System
                 : Binary Resources Archive
                 : C - Vars
                 : Save System

    And Beyond   : Physics System
                 : Networked Physics
                 : Networked Game Infrastructure
                 : Mods
```

## Current State

Currently, portal engine is at the end of the `MVP` phase of the roadmap.
The focus up until now has been on getting the basic infrastructure in place, and getting the engine to a point where it
will be straightforward to add new features.

## Detailed Roadmap

Here is a detailed list of planned features and modules.  
You can see more issues [here](https://github.com/JonatanNevo/portal-framework/issues?q=sort%3Aupdated-desc+is%3Aissue+is%3Aopen) and which issues are currently being worked on [here](https://github.com/users/JonatanNevo/projects/2).

### Basic Editor

**Phase:**  `Working Game`  
Like most game engines, Portal Engine should have a game editor to edit the scenes and resources.  
The current plan is to use `ImGui` as the base and create a generic and modular editor that will be extendable for
future features.

### Animation System

**Phase:**  `Working Game`  
Add an animation system, with all its dependencies.

### Audio System

**Phase:**  `Working Game`  
Add an audio system that supports both static audio and 3d dynamic audio.

### Advanced Shaders

**Phase:**  `Working Game`  
Implement advanced shaders (PBR, etc...) in slang, Using [Real Time Rendering](https://www.realtimerendering.com/) book as reference, and
extending the shader system to support multiple and dynamic shaders.

### Basic Networking

**Phase:**  `Working Game`  
Refactor the `networking` module to make it up to standard with the rest of the engine. 

### UI System

**Phase:**  `Working Game`  
Add a basic UI system to use in game (not based on `ImGui`).

### Scripting

**Phase:**  `Working Game`
Add scripting support to create and control game states, objects, entities and the like.
Currently the plan is to either use Lua, Python, C# or use WebAssembly to support all languages. Still under consideration.

### AI System

**Phase:**  `Working Game`
Add an AI system for creating intelligent game entities such as enemies and NPCs.
This will include pathfinding, behavior trees, navigation meshes, and decision-making systems.

### Binary Resources Archive
...

### C - Vars
...

### Save System
...

### Physics System

**Phase:**  `And Beyond`  
Implement a basic physics system, or include a physics engine like [Jolt](https://github.com/jrouwe/JoltPhysics)

### Networked Physics

**Phase:**  `And Beyond`  
Combine the physics system and networking to allow networked physics interactions.

### Networked Game Infrastructure

**Phase:**  `And Beyond`
Improve the networking system to allow replicated player movement, client-side prediction, server reconciliation,
and lag compensation.

### Mods

**Phase:**  `And Beyond`  
Implement a mod system using webassembly's sandbox.