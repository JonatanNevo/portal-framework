//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

/**
 * @file resource_registry.h
 * @brief Central resource manager for asynchronous asset loading
 *
 * This file defines the ResourceRegistry, the central manager for all resource loading
 * in the Portal Framework. It orchestrates async loading via the job system, manages
 * resource lifetime, and provides the primary API for requesting resources.
 *
 * @see ResourceReference for the user-facing handle API
 * @see ResourceDatabase for filesystem abstraction
 * @see LoaderFactory for loader selection
 */

#pragma once

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>
#include <portal/core/jobs/scheduler.h>

#include "portal/engine/resources/utils.h"
#include "reference_manager.h"
#include "database/resource_database.h"
#include "loader/loader_factory.h"
#include "portal/core/concurrency/spin_lock.h"
#include "portal/engine/reference.h"
#include "portal/engine/ecs/registry.h"
#include "portal/engine/modules/scheduler_module.h"
#include "portal/engine/resources/resources/resource.h"

namespace portal::renderer::vulkan
{
class VulkanTexture;
}

namespace portal
{
template <ResourceConcept T>
class ResourceReference;

/**
 * @class ResourceRegistry
 * @brief Central manager for asynchronous resource loading and lifetime management
 *
 * The ResourceRegistry is the core of the resource system. It provides the primary API for
 * loading resources (textures, meshes, materials, etc.) with asynchronous job system integration.
 *
 * Architecture:
 *
 * The registry is a Module that depends on:
 * - ReferenceManager: Tracks reference counts for future unloading support
 * - ResourceDatabase: Provides filesystem abstraction and metadata
 * - SchedulerModule: Job system for async loading
 * - SystemOrchestrator: ECS integration
 *
 * Internal State:
 *
 * The registry maintains three key data structures:
 * - `resources`: DenseMap of loaded resources (StringId → Reference<Resource>)
 * - `pending_resources`: DenseSet of resources currently loading
 * - `errored_resources`: DenseSet of resources that failed to load
 *
 * All access to these structures is protected by a SpinLock for thread safety.
 *
 * Loading Flow:
 *
 * 1. User calls load<T>(resource_id) → returns ResourceReference<T> immediately
 * 2. Registry checks if resource exists/is pending → early return if so
 * 3. Registry dispatches load_resource() coroutine to job system
 * 4. Coroutine queries database for metadata → creates source → loads via loader
 * 5. Resource moves from pending_resources to resources
 * 6. ResourceReference lazily discovers loaded resource when queried
 *
 * Resource Identity:
 *
 * Resources are identified by StringId (hash of the resource path):
 * - resource_id: The StringId used for lookups (e.g., STRING_ID("textures/albedo.png"))
 * - resource_handle: Internal handle (currently same as resource_id)
 *
 * Current Limitations:
 * - No unloading: Resources stay loaded forever (TODO at line 77)
 * - No streaming: Large resources must fit in memory
 * - No hot-reload: Changes require restart
 *
 * Usage Example (Async):
 * @code
 * // Request async load (returns immediately)
 * auto texture_ref = registry.load<TextureResource>(STRING_ID("textures/albedo.png"));
 *
 * // Poll each frame until ready
 * if (texture_ref.is_valid()) {
 *     auto& texture = texture_ref.get();
 *     renderer.bind_texture(texture);
 * }
 * @endcode
 *
 * Usage Example (Immediate):
 * @code
 * // Block until loaded (for startup resources)
 * auto mesh_ref = registry.immediate_load<MeshResource>(STRING_ID("models/character.gltf"));
 * // mesh_ref.is_valid() is guaranteed true here (or Error if failed)
 * @endcode
 *
 * @see ResourceReference for the handle API
 * @see ResourceDatabase for metadata and file access
 * @see LoaderFactory for loader selection
 * @see ReferenceManager for reference counting
 */
class ResourceRegistry
{
public:
    /**
     * @brief Constructor
     * @param ecs_registry
     * @param scheduler
     * @param database
     * @param reference_manager
     * @param context Vulkan context for resource creation
     */
    ResourceRegistry(
        const Project& project,
        ecs::Registry& ecs_registry,
        jobs::Scheduler& scheduler,
        ResourceDatabase& database,
        ReferenceManager& reference_manager,
        const renderer::vulkan::VulkanContext& context
    );
    ~ResourceRegistry() noexcept;

    /**
     * Request an asynchronous load for a resource based on its unique id and returns a reference.
     * The returned reference is invalid until the resource is loaded, once its loaded it can be accessed through the `ResourceReference` api
     *
     * @note Resources cannot have an `invalid state` but a reference can have one, make sure to test it before using the underlying resource.
     * @note the resource id is different from the resource handle, while both are unique per resource.
     *
     * @tparam T The underlying requested resource type
     * @param resource_id The resource id
     * @return A reference to the resource
     */
    template <ResourceConcept T>
    ResourceReference<T> load(StringId resource_id)
    {
        auto type = utils::to_resource_type<T>();
        create_resource(resource_id, type);

        auto reference = ResourceReference<T>(resource_id, *this, reference_manager);
        return reference;
    }

    /**
     * Request an immediate load of a resource based on its unique id and returns a reference to it.
     *
     * @tparam T The underlying requested resource type
     * @param resource_id The resource id
     * @return A reference to the resource
     */
    template <ResourceConcept T>
    ResourceReference<T> immediate_load(StringId resource_id)
    {
        create_resource_immediate(resource_id);

        auto reference = ResourceReference<T>(resource_id, *this, reference_manager);
        return reference;
    }

    void save(const StringId& resource_id);

    void load_snapshot(const StringId& resource_id, Buffer snapshot_data);
    Buffer snapshot(const StringId& resource_id);


    // TODO: Unload

    /**
     * Get a reference to an existing resource of type T, but does not attempt to create it if not loaded.
     * If the resource does not exist, returns a null reference.
     *
     * @tparam T The underlying requested resource type
     * @param resource_id The resource id
     * @return A reference to the resource, if exists
     */
    template <ResourceConcept T>
    ResourceReference<T> get(const StringId resource_id)
    {
        if (resources.contains(resource_id))
            return ResourceReference<T>(resource_id, *this, reference_manager);

        auto res = database.find(resource_id);
        if (res.has_value())
            return ResourceReference<T>(res->resource_id, *this, reference_manager);

        return ResourceReference<T>(INVALID_STRING_ID, *this, reference_manager);
    }

    /**
     * Allocated a resource of type T in the registry, returns a pointer to it (not a `ResourceReference`)
     * Resources allocated this way are considered loaded and can be used straight away
     *
     * @tparam T The type of the resource
     * @param id A handle to the resource
     * @param args The constructor arguments for the resource
     * @return A pointer to the resource
     */
    template <ResourceConcept T, typename... Args>
    Reference<T> allocate(const StringId id, Args&&... args)
    {
        // TODO: add some dependency checks?
        auto ref = make_reference<T>(std::forward<Args>(args)...);

        std::lock_guard guard(lock);
        resources[id] = {ref};
        return ref;
    }

    /**
     * Directly loads a resource from a source (through the corresponding loader)
     *
     * @param meta The source metadata
     * @param source A type erased source
     * @return The created resource data
     */
    Job<resources::ResourceData> load_direct(SourceMetadata meta, Reference<resources::ResourceSource> source);

    // TODO: remove from here
    void wait_all(std::span<Job<>> jobs) const;

    template <typename T>
    auto list_all_resources_of_type()
    {
        return resources | std::ranges::views::filter(
            [](auto& it)
            {
                auto& [name, resource_data] = it;
                auto typed_resource = reference_cast<T>(resource_data.resource);
                return typed_resource != nullptr;
            }
        ) | std::ranges::views::transform(
            [this](auto& it)
            {
                return this->get<T>(it.first);
            }
        );
    }

    [[nodiscard]] ecs::Registry& get_ecs_registry() const { return ecs_registry; }
    [[nodiscard]] const Project& get_project() const { return project; }
    [[nodiscard]] ResourceDatabase& get_resource_database() const { return database; }

    void save_resource(resources::ResourceData& resource_data);
    Buffer snapshot_resource(const resources::ResourceData& resource_data);

protected:
    /**
     * Gets a pointer to the resource from a handle, if the resource is invalid, returns the invalid state instead
     *
     * @param id The handle to the resource
     * @return A resource pointer if valid, the invalid state as a `ResourceState` enum otherwise
     */
    [[nodiscard]] std::expected<Reference<Resource>, ResourceState> get_resource(const StringId& id);

    /**
     * Creates a new resource asynchronously in the registry and returns a handle to it.
     * If the resource exists already (either in pending or loaded), returns the existing handle
     *
     * @param resource_id The resource id.
     * @param type The type of the resource.
     * @return A handle to the resource
     */
    void create_resource(const StringId& resource_id, ResourceType type);

    /**
     * Much like the `create_resource` this function creates a resource and returns a hanlde, but blocks until the resource creation is done.
     * If the resource exists already (either in pending or loaded), returns the existing handle
     *
     * @param resource_id The resource id.
     * @return A handle to the resource
     */
    void create_resource_immediate(const StringId& resource_id);

    Job<Reference<Resource>> load_resource(StringId handle);

private:
    template <ResourceConcept T>
    friend class ResourceReference;

    void set_dirty(const StringId& resource_id, ResourceDirtyFlags flags);
    ResourceDirtyFlags get_dirty(const StringId& resource_id);

    const Project& project;
    ecs::Registry& ecs_registry;
    jobs::Scheduler& scheduler;
    ResourceDatabase& database;
    ReferenceManager& reference_manager;

    SpinLock lock;
    // Resource container, all resource are managed
    // TODO: use custom allocator to have the resources next to each other on the heap
    std::unordered_map<StringId, resources::ResourceData> resources;
    llvm::DenseSet<StringId> pending_resources;
    llvm::DenseSet<StringId> errored_resources;

    resources::LoaderFactory loader_factory;
};
} // portal


