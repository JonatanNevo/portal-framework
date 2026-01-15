//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <any>
#include <portal/core/strings/string_id.h>

#include "portal/application/modules/tags.h"

namespace portal
{
struct FrameContext;

class Event;

/**
 * Polymorphic base class for all modules in the application.
 *
 * BaseModule provides the common interface for all modules, defining virtual lifecycle
 * hooks that modules can override to participate in frame execution. All lifecycle methods
 * are no-ops by default; modules only override the hooks they need.
 *
 * Derived classes (typically TaggedModule) implement has_tag() to declare which lifecycle
 * phases they participate in, enabling the ModuleStack to organize execution efficiently.
 */
class BaseModule
{
public:
    explicit BaseModule(const StringId& name) : name(name) {}
    virtual ~BaseModule() = default;

    /**
     * Called at the beginning of each frame.
     * Override to perform per-frame initialization.
     * @param frame Per-frame context data
     */
    virtual void begin_frame(FrameContext& frame);

    /**
     * Called during the update phase for game logic.
     * @param frame Per-frame context data
     */
    virtual void update(FrameContext& frame);

    /**
     * Called during the post-update phase (typically rendering).
     * @param frame Per-frame context data
     */
    virtual void post_update(FrameContext& frame);

    /**
     * Called at the end of each frame in reverse dependency order.
     * Override to perform per-frame cleanup.
     * @param frame Per-frame context data
     */
    virtual void end_frame(FrameContext& frame);

    /**
     * Called during the GUI update phase.
     * @param frame Per-frame context data
     */
    virtual void gui_update(FrameContext& frame);

    /**
     * Called when events are dispatched to modules.
     * @param event The event to process
     */
    virtual void on_event(Event& event);

    /** @return The module's name for debugging and introspection */
    [[nodiscard]] const StringId& get_name() const { return name; }

    /** @return Vector of modules this module depends on (empty by default) */
    [[nodiscard]] virtual std::vector<BaseModule*> get_dependencies() const { return {}; }

    /**
     * Check if this module has a specific tag.
     * @tparam T The ModuleTags value to check
     * @return true if this module participates in the specified lifecycle phase
     */
    template <ModuleTags T>
    bool has_tag()
    {
        return has_tag(Tag<T>::Tags);
    }

    /**
     * Check if this module has all specified tags.
     * @tparam T Variadic pack of ModuleTags to check
     * @return true if this module participates in all specified lifecycle phases
     */
    template <ModuleTags... T>
    bool has_tags()
    {
        std::array query = {Tag<T>::Tags...};

        bool res = true;
        for (auto id : query)
        {
            res &= has_tag(id);
        }
        return res;
    }

    /**
     * Check if this module has a specific tag (runtime version).
     * Pure virtual - implemented by TaggedModule.
     * @param tag The TagFlag to check
     * @return true if this module participates in the specified lifecycle phase
     */
    [[nodiscard]] virtual bool has_tag(TagFlag tag) const = 0;

protected:
    StringId name;
};
}
