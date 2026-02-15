# Architecture


## Engine Architecture

There were a few design goals I had while designing the engine:  
1. The engine should be generic and [extendable](#extendable-engine).  
2. The engine should be [easy to use](#easy-to-use) and navigate.  

 ---

## Extendable Engine

The engine and framework are built with extensibility in mind; to implement this, there are a few levels of abstraction and indirection to allow both expansion and testability.  

### Abstraction Levels
```{mermaid}
---
caption: Abstraction Levels with some examples
---
flowchart TD

Game --> Engine
Engine --> Application
Engine --> Renderer

subgraph Game
    direction TB
    g[Gameplay Systems]
    s[Runtime/ Compile Settings]
end

subgraph Engine
    direction TB
    Resources
    ECS
end

subgraph Application
    direction TB
    j[Job System]
    l[Lifetime Management]
end

subgraph Renderer
    direction TB
    v[Vulkan Abstraction]

end
```

#### Game Level

The highest level of abstraction is the game level.  
This is where all the user's code lives, basically everything that exists outside of the engine package, such as the gameplay systems, runtime settings, etc...  

```{important}
In the current state of the framework, this level is the least implemented and will likely change the most in the near future.
The effort so far was to implement the layers below.
```

##### Usage Example
```{code-block} cpp
// The entry point of the portal framework's application
std::unique_ptr<Application> portal::create_application(int argc, char** argv)
{
    // Configures basic settings
    const auto prop = ApplicationProperties::from_settings();
    auto engine = std::make_unique<Engine>(prop);
    
    // Register and configure gameplay systems
       
    // Returns a pointer to the application to run
    return engine;
}
```
#### Engine Level

The engine level is in charge of orchestrating all the different modules and systems.   
The main job of the engine is to initialize the modules and systems in the required order with the correct dependencies.    
Also, the ECS (entity component system) lives in this level of abstraction.  

##### Entity Component System

For those unfamiliar with ECS, you can read more about it [here](https://en.wikipedia.org/wiki/Entity_component_system).  
The ECS's job in Portal Engine is to handle all the real-time operations, both engine-specific and game-specific, for example:   
* Updating the scene graph transforms when something moves.  
* Querying the scene graph for entities to render this frame.  
* Going through all physics enabled entities and calculating the physics simulation.  

The users are able to easily add new types of components and systems to their game, for example:
```{code-block} cpp
// Creates a new entity with the name "Player"
auto player = scene->get_registry().create_entity(STRING_ID("Player"));

// Tags the entity as a player
player.add_component<PlayerTag>();

// Adds an input component to the entity, the `PlayerInputSystem` will now get this 
// entity when it wants to update the input states.
player.add_component<InputComponent>();

// Adds a camera to the entity and tags it as the main camera
player.add_component<CameraComponent>();
player.add_component<MainCameraTag>();

// Adds a controller to the entity, the controller receives input from the 
// `InputComponent` and updates the camera's transform.
player.add_component<BaseCameraController>();
```

#### Application Level

The application level is in charge of managing the lifetime of the engine, which means handling both initialization and the game loop itself.  

##### Application Modules

To have the application control the lifetime of objects, the object needs to implement the `Module` interface.  
When implementing this interface, you can specify both the dependencies of the module and the lifetime hooks of the module.    
For example:  

```{code-block} cpp
// Specifies a new module `EditorModule` that depends on
// the `Renderer` and `SystemOrchestrator` modules.

// Additionally, specifies the tag `ModuleTags::GuiUpdate` 
// which means that the application system will call this class's
// `gui_update(FrameContext&)` function every frame.
class EditorModule final 
: public TaggedModule<
    Tag<ModuleTags::GuiUpdate>,
    Renderer, SystemOrchestrator
  >
{
...
```

#### Renderer Level

At the lowest level of abstraction lies the renderer, which abstracts away the Vulkan API and provides a generalized interface for GPU operations and rendering.  
While currently only a Vulkan renderer is implemented, the idea is to build the renderer in a way that it can be easily extended to support other graphics APIs.  


### How it all comes together

Due to the abstraction level design, it is quite easy to add new features and systems to the engine.  
The application modules, the systems, and the framework modules all work together seamlessly.  

For example, if I want to add a physics system to the engine, I can simply add a new framework module, implement a new physics system there, and add it to the engine's system orchestrator.  

## Easy To Use

The code is designed to be easy to navigate and understand.  
Here are a few concepts that I think it's important to note:  

### Contexts Instead Of Singletons
One of the major pain-points I have with most game engine implementations I saw is that there are a large number of singletons.  
Which makes following the flow of ownership, and the lifetime of objects challenging.  
Instead of using singletons, I opted to use layered contexts, where each "layer" in the abstraction level gets its own context, and objects in the same layer have access to all contexts in the same layer or below.  
  
Additionally, each frame in flight gets its own context, which allows for better isolation and synchronization of resources and operations across frames. This approach also simplifies the management of resources and reduces the risk of race conditions and data corruption.  

#### Context Hierarchy
```{mermaid}
flowchart TD

subgraph Static Contexts
    direction TB
    EngineContext --> RendererContext
    RendererContext --> VulkanContext
end

subgraph Realtime Contexts
    direction TB
    FrameContext --> RenderingContext   
end
```

### CMake Made Easy
To make the framework and engine easy to use and cross-platform, I've added a set of cmake functions that makes it easy to add framework modules and games using the engine.  

#### Adding A New Framework Module
To add a new framework module, you need to use two cmake functions:

```{code-block} cmake
# Adds a new `portal-application` target, with the given sources and headers.
portal_add_module(application
    SOURCES ${APP_SOURCES}
    HEADERS ${APP_HEADERS}
    
    # Specifies the portal dependencies of the module.
    PORTAL_DEPENDENCIES
        core
        serialization
        
    # There is an optional `DEPENDENCIES` argument to specify,
    # Non portal dependencies of the module.
    
    # Specifies a config file that should be filled out in compile time with the required settings
    # when attempting to build a binary with this module
    COMPILE_CONFIG_FILE
        portal/application/config.h
)

# Tells cmake that we want to install this target, and configures all install rules.
portal_install_module(application)
```
To read more, see [portal_add_module](../api/cmake/portal-add-module.rst)

#### Making A Game With The Engine

To create a game with the engine, you need to call a single cmake function:

```{code-block} cmake
# Configures a target with the name `my-game`
# (in the future will configure a target called `my-game-editor` as well)
portal_add_game(my-game
        SOURCES source/main.cpp # A list of sources files to compile
        DISPLAY_NAME "My Game"
        VENDOR "Jonatan Nevo"
        CONTACT "jonatannevo-git@proton.me"
)
```

If you also want to package an installer for distribution, you can add the following:
```{code-block} cmake
# Configures a QT Installer Framework installer for the game `my-game`
portal_game_configure_installer(
        my-game
)
```
To read more, see [portal_add_game](../api/cmake/portal-add-game.rst)

