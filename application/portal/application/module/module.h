//
// Created by Jonatan Nevo on 01/03/2025.
//

#pragma once
#include <deque>
#include <string>
#include <vector>

#include <portal/core/tags.h>

#include "portal/core/assert.h"

namespace portal
{
namespace debug
{
    class DebugInfo;
}

class Configuration;
class Platform;
class Module;

/**
 * @brief Tags are used to define a module's behaviour. This is useful to dictate which modules will work together
 * 	      and which will not without directly specifying an exclusion or inclusion list. Tags are struct types so that they can
 * 		  be used in the tagging system (See Module implementation).
 *
 * Rendering -   A rendering tag is used to define a module that will render to the screen, also adds more hooks for the module
 */
namespace tags
{
    struct Rendering {};
} // namespace tags

/**
 * @brief Module are used to define custom behaviour. This allows the addition of features without directly
 * 		  interfering with the applications core implementation
 */
class Module
{
public:
    /**
     * @brief Hooks are points in the project that a module can subscribe too. These can be expanded on to implement more behaviour in the future
     *
     * Update -          Executed at each update() loop
     * OnAppStart -      Executed when an app starts
     * OnAppClose -      Executed when an app closes
     *
     * PostDraw -        Executed after the draw() loop
     * OnUpdateUi -      Executed after the update_ui() loop
     */
    enum class Hook
    {
        OnAppStart,
        OnAppClose,
        OnUpdate,
        OnAppError,

        OnResize,

        // Rendering specific hooks
        OnUpdateUi,
        PostDraw
    };

public:
    explicit Module(
        std::string name
    ): name(std::move(name)) {}

    virtual ~Module() = default;

    /**
     * @brief Return a list of hooks that a module wants to subscribe to
     *
     * @return Hooks that the module wants to use
     */
    [[nodiscard]] virtual const std::vector<Hook>& get_hooks() const = 0;

    /**
     * @brief Called when an application has been updated
     *
     * @param delta_time The time taken to compute a frame
     */
    virtual void on_update(float delta_time) = 0;

    /**
     * @brief Called when an app has started
     */
    virtual void on_start(const Configuration& config, debug::DebugInfo& debug_info) = 0;

    /**
     * @brief Called when an app has been closed
     */
    virtual void on_close() = 0;

    /**
     * @brief Handle when an application errors
     */
    virtual void on_error() = 0;

    /**
     * @brief Handle when a window resize request arrives
     */
    virtual void on_resize(uint32_t width, uint32_t) = 0;

    /**
     * @brief Post Draw
     */
    virtual void on_post_draw(vulkan::rendering::RenderContext& context) = 0;

    /**
     * @brief Allows to add a UI to a sample
     *
     * @param drawer The object that is responsible for drawing the overlay
     */
    virtual void on_update_ui(gui::Drawer& drawer) = 0;

    [[nodiscard]] const std::string& get_name() const { return name; }

    /**
     * @brief Test whether the module contains a given tag
     *
     * @tparam C the tag to check for
     * @return true tag present
     * @return false tag not present
     */
    template <typename C>
    [[nodiscard]] bool has_tag() const
    {
        return has_tag(Tag<C>::ID);
    }

    /**
     * @brief Tests whether the module contain multiple tags
     *
     * @tparam C A set of tags
     * @return true Contains all tags
     * @return false Does not contain all tags
     */
    template <typename... C>
    [[nodiscard]] bool has_tags() const
    {
        const std::vector<TagID> query = {Tag<C>::ID...};
        bool res = true;
        for (const auto id : query)
        {
            res &= has_tag(id);
        }
        return res;
    }

    /**
     * @brief Implemented by module base to return if the module contains a tag
     *
     * @param id The tag id of a tag
     * @return true contains tag
     * @return false does not contain tag
     */
    virtual bool has_tag(TagID id) const = 0;

    void set_platform(Platform* platform)
    {
        PORTAL_CORE_ASSERT(!this->platform && platform, "Platform already set or platform is null");
        this->platform = platform;
    }

protected:
    Platform* platform = nullptr;

private:
    std::string name;
};


namespace modules
{
    /**
 * @brief Get all modules with tags
 * 		  Plugin must include one or more tags
 *
 * @tparam TAGS Tags that a module must contain
 * @param domain The list of modules to query
 * @return const std::vector<Module*> A list of modules containing one or more TAGS
 */
    template <typename... TAGS>
    std::vector<Module*> with_tags(const std::vector<Module*>& domain = {})
    {
        const std::vector<TagID> tags = {Tag<TAGS>::ID...};
        std::vector<Module*> compatible;
        for (auto ext : domain)
        {
            PORTAL_CORE_ASSERT(ext != nullptr, "Module is null");

            bool has_one = false;
            for (const auto t : tags)
            {
                has_one |= ext->has_tag(t);
            }

            if (has_one)
                compatible.push_back(ext);
        }
        return compatible;
    }


    /**
     * @brief Get all modules without the given tags
     * 		  Plugin must not include one or more tags
     * 		  Essentially the opposite of modules::with_tags<...TAGS>()
     *
     * @tparam TAGS Tags that a module must not contain
     * @param domain The list of modules to query
     * @return const std::vector<Plugin*> A list of modules containing one or more TAGS
     */
    template <typename... TAGS>
    std::vector<Module*> without_tags(const std::vector<Module*>& domain = {})
    {
        const std::vector<TagID> tags = {Tag<TAGS>::ID...};
        std::vector<Module*> compatible;
        for (auto ext : domain)
        {
            PORTAL_CORE_ASSERT(ext != nullptr, "Plugin is null");

            bool has_any = false;
            for (const auto t : tags)
            {
                has_any |= ext->has_tag(t);
            }

            if (!has_any)
                compatible.push_back(ext);
        }
        return compatible;
    }
}
} // portal
