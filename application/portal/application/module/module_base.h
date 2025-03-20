//
// Created by Jonatan Nevo on 01/03/2025.
//

#pragma once

#include <portal/core/tags.h>

#include "module.h"

namespace portal
{
namespace gui {
    class Drawer;
}

namespace vulkan::rendering {
    class RenderContext;
}

/**
 * @brief ModuleBase is the base class that modules inherit from. The class enforces the use of tags when creating new modules.
 * 		  For method information see Module
 */
template <typename... TAGS>
class ModuleBase : public Module, public Tag<TAGS...>
{
public:
    explicit ModuleBase(
        const std::string& name,
        const std::vector<Hook>& hooks = {}
    ): Module(name), hooks{hooks} {}

    [[nodiscard]] const std::vector<Hook>& get_hooks() const override { return hooks; }
    bool has_tag(TagID id) const override { return tags->has_tag(id); }

    // hooks that can be implemented by modules
    void on_update(float delta_time) override {};
    void on_start(const Configuration& config, debug::DebugInfo& debug_info) override {};
    void on_close() override {};
    void on_error() override {};
    void on_resize(uint32_t width, uint32_t height) override {};
    void on_post_draw(vulkan::rendering::RenderContext &context) override{};
    void on_update_ui(gui::Drawer &drawer) override{};

private:
    Tag<TAGS...>* tags = reinterpret_cast<Tag<TAGS...>*>(this);
    std::vector<Hook> hooks;
};
}
