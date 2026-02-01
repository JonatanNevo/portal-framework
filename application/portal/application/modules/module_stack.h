//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <memory>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "portal/application/frame_context.h"
#include "portal/application/modules/base_module.h"

namespace portal
{
class Event;

// TODO: find a better name?
// TODO: differentiate between using modules for dependency graph and memory allocation to using modules for polymorphisem flow control
/**
 * Dependency injection container managing module lifetime and execution.
 *
 * ModuleStack is the central registry for all modules in a Portal application.
 * It handles module registration, dependency resolution through topological sorting,
 * and provides lifecycle facade methods that execute modules in dependency order.
 *
 * After modules are registered with add_module(), call build_dependency_graph() to
 * perform topological sorting. This organizes modules by dependency level and creates
 * pre-sorted vectors for each tag type.
 *
 * The lifecycle facade methods (begin_frame, update, etc.) iterate the appropriate
 * tag-specific vectors, ensuring modules execute in dependency order with zero
 * runtime overhead for resolution.
 *
 * Example usage:
 * @code
 * ModuleStack modules;
 * modules.add_module<Renderer>(STRING_ID("Renderer"));
 * modules.add_module<ResourceRegistry>(STRING_ID("Resources"));
 * modules.add_module<GameLogic>(STRING_ID("Game"));
 * modules.build_dependency_graph();  // Must call after registration
 *
 * // In game loop:
 * // Calls update() on modules in dependency order
 * modules.update(frame_context);
 * @endcode
 */
class ModuleStack
{
public:
    ~ModuleStack();

    /**
     * Clean up all modules in reverse dependency order.
     * Called by destructor, but can be called manually for explicit cleanup.
     */
    void clean();

    /**
     * Register a new module and construct it in-place.
     *
     * The module receives a reference to this ModuleStack as its first constructor
     * argument, followed by any additional arguments. During construction, the module's
     * dependencies are resolved using ModuleLookup.
     *
     * Marks the dependency graph as dirty, requiring rebuild before execution.
     *
     * @tparam T The module type, must inherit from BaseModule
     * @tparam Args Additional constructor argument types
     * @param args Arguments forwarded to the module's constructor (after ModuleStack&)
     * @return Reference to the newly constructed module
     * @throws std::runtime_error if module dependencies cannot be resolved
     */
    template <typename T, typename... Args> requires std::is_base_of_v<BaseModule, T>
    T& add_module(Args&&... args)
    {
        auto& module = modules.emplace_back(std::make_unique<T>(*this, std::forward<Args>(args)...));
        dependency_graph_dirty = true;
        return dynamic_cast<T&>(*module);
    }

    /**
     * Get a span of all registered modules for introspection.
     * @return Span of unique pointers to all registered modules
     */
    std::span<std::unique_ptr<BaseModule>> list_modules()
    {
        return modules;
    }

    /**
     * Perform topological sort of modules based on dependencies.
     *
     * This method organizes modules into dependency levels using a depth-first search
     * algorithm. Modules with no dependencies are level 0, and each module's level is
     * one plus the maximum level of its dependencies.
     *
     * The sorted modules are organized into:
     * - dependency_graph: vector-of-vectors where each inner vector is a dependency level
     * - Tag-specific vectors (update_modules, etc.): pre-sorted for efficient iteration
     *
     * Must be called after module registration and before execution.
     *
     * @throws std::runtime_error if circular dependencies are detected
     */
    void build_dependency_graph();


    /**
     * Call begin_frame() on all FrameLifecycle modules in dependency order.
     * @param frame Per-frame context data
     */
    void begin_frame(FrameContext& frame) const;

    /**
     * Call end_frame() on all FrameLifecycle modules in reverse dependency order.
     * @param frame Per-frame context data
     */
    void end_frame(FrameContext& frame) const;

    /**
     * Call update() on all Update-tagged modules in dependency order.
     * @param frame Per-frame context data
     */
    void update(FrameContext& frame) const;

    /**
     * Call post_update() on all PostUpdate-tagged modules in dependency order.
     * @param frame Per-frame context data
     */
    void post_update(FrameContext& frame) const;

    /**
     * Call gui_update() on all GuiUpdate-tagged modules in dependency order.
     * @param frame Per-frame context data
     */
    void gui_update(FrameContext& frame) const;

    /**
     * Get the computed dependency graph for introspection.
     * @return Vector of dependency levels, where each level is a vector of modules
     */
    [[nodiscard]] const std::vector<std::vector<BaseModule*>>& get_dependency_graph() const { return dependency_graph; }

private:
    /**
     * Recursively compute a module's dependency level using depth-first search.
     *
     * @param module The module to compute level for
     * @param levels Map of already-computed levels
     * @param visited Set tracking currently visiting modules (for cycle detection)
     * @return The dependency level (0 for no deps, 1 + max(dependency levels) otherwise)
     * @throws std::runtime_error if a circular dependency is detected
     */
    static int compute_module_level(
        BaseModule* module,
        std::unordered_map<BaseModule*, int>& levels,
        std::unordered_set<BaseModule*>& visited
    );

private:
    std::vector<std::unique_ptr<BaseModule>> modules;
    std::vector<std::vector<BaseModule*>> dependency_graph;
    bool dependency_graph_dirty = false;

    // Pre-sorted vectors for efficient tag-based iteration
    // TODO: Split into multiple subclasses instead of having everything inside base_module
    std::vector<BaseModule*> frame_lifecycle_modules;
    std::vector<BaseModule*> update_modules;
    std::vector<BaseModule*> gui_update_modules;
    std::vector<BaseModule*> post_update_modules;
};
} // portal
