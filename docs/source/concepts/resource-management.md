# Resource Management

```{mermaid}
---
caption: Resource System High Level Design
---
flowchart TD
    ResourceRegistry[Resource Registry] --> LoaderFactory[Loader Factory]
    ResourceRegistry --> ResourceDatabase[Resource Database]
    ResourceRegistry --> ReferenceManager[Reference Manager]
    
    ResourceRegistry -. creates .-> ResourceReference[Resource Reference]
    ResourceReference -- registers/unregisters --> ReferenceManager
    
    LoaderFactory -- creates --> Loader[Loader]
    Loader -- uses --> ResourceSource[Resource Source]
    
    ResourceDatabase -- creates --> ResourceSource
```

The resource management system is responsible for loading, streaming, and saving assets.
It manages the lifetime of resources on both the GPU and CPU.

## Resource Registry

The [portal::Resource Registry](exhale_class_classportal_1_1ResourceRegistry) is the central point and the public API
for all resource-related functionality.
It exposes functions to load, save, and retrieve resources. Additionally, it provides an `allocate` method for programmatically creating resources that are not backed by a source file (e.g., procedural textures).

The resource registry is built on top of the [portal::jobs::Scheduler](exhale-class-classportal-1-1jobs-1-1Scheduler) to
utilize the job system and parallel resource loading.

The resource registry holds ownership over all resources in the system.

## Resource Database

The resource database is an interface for creating a [ResourceSource](#resource-source) based on an ID and metadata.
When applicable, it is also in charge of maintaining metadata and providing an interface for adding and removing resources.

### Resource Database Implementations

```{mermaid}
flowchart TD
DB[Resource Database] --- FDB[Folder Database]
DB[Resource Database] --- BDB[Bundle Database]
```

### Resource IDs

Resources are identified by a unique string ID. By convention, this ID is formatted as `database_name/resource_path`.
The `database_name` is defined in the `.podb` file, and the `resource_path` typically matches the relative path of the source file within the database's root folder (excluding the extension).

#### Folder Database

The folder database is a mutable implementation used for development and debugging.
To define a folder database, specify a root folder where the resources are located.

````{dropdown} settings.json example
:open:
```json5
{
  "project": {
    "resources": [
      {
        "path": "resources", // either a relative path (relative to the project root) or an absolute path
        "type": "Folder"
      }      
    ],
    // ...
  },
  // ...
}
```
````

The database expects a database file (`.podb`) to exist within the resource folder root.

````{dropdown} .podb file example
```json5
{
    "dirty": 0, // Dirty flag, do not modify yourself
    "ignored_extensions": [ // a list of file extensions to ignore
        ".bin"
    ],
    "ignored_files": [ // a list of files to ignore
        ".gitkeep"
    ],
    "name": "game", // The name of the database, used for generating resource ids
    "resource_count": 59, // The number of resources detected in the database
    "version": 1 // The version of the database format
}
```
````

The database will ensure that all detected resources exist in the database and have a corresponding meta file (
`.pmeta`); if not, it will create one.

````{dropdown} .pmeta file example
```json5
{
    "dependencies": [], // A list of resource ids that this resource depends on
    "format": "Image", // The format of the resource 
    "resource_id": "game/Bishop_black_base_color", // The ID of the resource (note that it's prepended by the database name) 
    "source": "Bishop_black_base_color.jpg", // The path to the source file
    // Resource specific metadata, in this case the texture metadata. 
    // This can be changed per `type` and per `format`
    "texture": { 
        "format": "RGBA8_UNorm",
        "hdr": false,
        "height": 2048,
        "width": 2048
    },
    "type": "Texture" // The type of the resource
}
```
````

#### Composite Resources

Some resources are "composites," meaning they contain or reference other sub-resources. A common example is a GLTF file, which can be treated as a single composite resource that, when loaded, populates the registry with multiple sub-resources (meshes, materials, textures). 

Composite metadata in the `.pmeta` file allows the system to track these internal dependencies and ensures that all parts of a complex asset are correctly identified by the database.

#### Bundle Database

Not yet implemented; see [Bundles](#bundles).

## Resource Source

The resource source is an interface for loading, streaming, and saving resources.
It is stateless by itself, created by the [Resource Database](#resource-database), and used by
the [Loaders](#loaders) to load resources.

### Resource Source Implementations

```{mermaid}
flowchart TD
Source[Resource Source] --- FS[File Source]
Source --- NS[Network Source]
Source --- BS[Bundle Source]
```

### File Resource Source

This is the simplest source implementation. It loads resources from a file, either reading the entire file into
memory or streaming it.

### Network Resource Source

Not yet implemented; in the future, this will allow loading resources over a network socket.

### Bundle Resource Source

Not yet implemented; in the future, this will allow loading resources from a bundle. See [Bundles](#bundles).

## Loaders

Loaders are responsible for reading raw resource data from the [Resource Source](#resource-source) and loading it into
memory.
They do not handle reference counting; their responsibility is limited to data integrity and formatting.

In addition to initial loading, loaders support **snapshot loading**, which allows restoring a resource's state from a memory buffer. This is used for features like hot-reloading or state serialization.

The resource loaders are stateless and created by the [Resource Registry](#resource-registry) through
a [portal::resources::LoaderFactory](exhale_class_classportal_1_1resources_1_1LoaderFactory).

## Resource References

When a resource is loaded into memory, it is stored in the [Resource Registry](#resource-registry).
When a user wants to use a resource, they request it from the registry and receive
a [portal::ResourceReference](exhale_class_classportal_1_1ResourceReference).

A resource itself cannot be null; it either exists in memory in a valid state or does not exist at all.
The `ResourceReference` acts as a "promise" for a resource. You can query its state via `get_state()`, which returns a `portal::ResourceState`:

- **Unknown**: Initial state before querying the registry.
- **Pending**: Resource is currently being loaded asynchronously by the job system.
- **Loaded**: Resource is fully loaded and ready for use. This is the only state where `get()` returns a valid pointer.
- **Unloaded**: Resource exists in the database but is not currently in memory.
- **Missing**: Resource ID was not found in the database.
- **Error**: Loading failed (e.g., corrupted file, out of memory).
- **Null**: The reference itself is invalid (default-constructed or moved-from).

Additionally, using RAII semantics, it signals the [Reference Manager](#reference-manager) when you acquire a reference
and when you discard it.
While the resource itself does not guarantee thread safety (depending on the resource implementation), the reference's state
query is thread safe.

### Resource Dirty Flags

Resources can track changes to their state using `ResourceDirtyFlags`. This is useful for systems like the renderer to know when a texture needs to be re-uploaded to the GPU.

- **Clean**: No changes.
- **StateChange**: General state change.
- **DataChange**: Raw data changed (e.g., pixels in a texture).
- **ConfigChange**: Metadata or configuration changed.

## Reference Manager

The reference manager is responsible for tracking all active `ResourceReference` instances. It does not own the resource memory (which is managed by the [Resource Registry](#resource-registry)), but it maintains a mapping of resource IDs to the sets of references pointing to them.

This information is crucial for the planned garbage collection system, allowing the registry to identify resources with zero active references that can be safely unloaded.

## Current Limitations and the Future

### Bundles

Currently, only the folder resource database is supported.
While great for debugging and development, when packaging a game, we prefer an immutable bundle of all required
resources (or multiple bundles).

Even though it is not yet implemented, the current architecture allows for easy extension of
the [portal::ResourceDatabase](exhale_class_classportal_1_1ResourceDatabase)
and [portal::resources::ResourceSource](exhale_class_classportal_1_1resources_1_1ResourceSource) interfaces to support
bundles without changing any API or existing implementations. It is planned for the near future to implement a bundle
system using [portal::BinarySerializer](exhale_class_classportal_1_1BinarySerializer).

### Streaming Large Resources

While [ResourceSource](#resource-source) supports streaming, currently the loaders do not support streaming of large
resources.
If you have very large resources, it is advised to split them into smaller resources (if possible) to allow better
concurrency while loading.

This is a current limitation that I am planning to solve in the future.

### Resource Unloading

Currently, resource unloading is not implemented; once a resource is loaded, it remains in memory.
Because current tests use small, static scenes, this is not an issue, but it will become one soon.

The plan is to implement periodic garbage collection based on reference counts. When a reference count reaches zero, the resource
will enter a `clean` collection.
Periodically (or when memory is needed), this collection will be queried, and resources whose reference counts are still
zero will be unloaded.

This system can be extended into a caching system—for example, unloading from the GPU but not the CPU because there
is more memory available on the CPU.