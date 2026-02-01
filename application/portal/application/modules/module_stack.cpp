//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "module_stack.h"

#include <algorithm>
#include <ranges>
#include <stdexcept>

#include "portal/application/modules/base_module.h"
#include "portal/core/debug/assert.h"

namespace portal
{
ModuleStack::~ModuleStack()
{
    clean();
}

void ModuleStack::clean()
{
    // delete modules in reverse order of dependency
    for (auto& module : modules | std::views::reverse)
        module.reset();
    modules.clear();
}


void ModuleStack::build_dependency_graph()
{
    dependency_graph.clear();

    std::unordered_map<BaseModule*, int> module_levels;
    std::unordered_set<BaseModule*> visited;

    for (auto& module : modules)
    {
        compute_module_level(module.get(), module_levels, visited);
    }

    int max_level = 0;
    for (const auto& level : module_levels | std::views::values)
    {
        max_level = std::max(max_level, level);
    }

    dependency_graph.resize(max_level + 1);
    for (const auto& [module, level] : module_levels)
    {
        dependency_graph[level].push_back(module);
    }

    std::ranges::sort(
        modules,
        [&module_levels](const auto& a, const auto& b)
        {
            return module_levels[a.get()] < module_levels[b.get()];
        }
    );

    // Going over sorted modules to retain order
    update_modules.clear();
    frame_lifecycle_modules.clear();
    gui_update_modules.clear();
    post_update_modules.clear();

    for (auto& module : modules)
    {
        if (module->has_tag<ModuleTags::Update>())
            update_modules.emplace_back(module.get());
        if (module->has_tag<ModuleTags::FrameLifecycle>())
            frame_lifecycle_modules.emplace_back(module.get());
        if (module->has_tag<ModuleTags::GuiUpdate>())
            gui_update_modules.emplace_back(module.get());
        if (module->has_tag<ModuleTags::PostUpdate>())
            post_update_modules.emplace_back(module.get());
    }
}

void ModuleStack::begin_frame(FrameContext& frame) const
{
    for (const auto& module : frame_lifecycle_modules)
    {
        module->begin_frame(frame);
    }
}

void ModuleStack::end_frame(FrameContext& frame) const
{
    // TODO: do we care about the cache misses here in the reverse order?
    for (const auto& module : frame_lifecycle_modules | std::views::reverse)
    {
        module->end_frame(frame);
    }
}

void ModuleStack::update(FrameContext& frame) const
{
    for (const auto& module : update_modules)
    {
        module->update(frame);
    }
}

void ModuleStack::gui_update(FrameContext& frame) const
{
    for (const auto& module : gui_update_modules)
    {
        module->gui_update(frame);
    }
}

void ModuleStack::post_update(FrameContext& frame) const
{
    for (const auto& module : post_update_modules)
    {
        module->post_update(frame);
    }
}


int ModuleStack::compute_module_level(BaseModule* module, std::unordered_map<BaseModule*, int>& levels, std::unordered_set<BaseModule*>& visited)
{
    if (const auto it = levels.find(module); it != levels.end())
    {
        return it->second;
    }

    if (visited.contains(module))
    {
        throw std::runtime_error("Circular dependency detected");
    }

    visited.insert(module);

    const auto deps = module->get_dependencies();

    if (deps.empty())
    {
        levels[module] = 0;
        visited.erase(module);
        return 0;
    }

    // Level is 1 + max level of dependencies
    int max_dep_level = -1;
    for (auto* dep : deps)
    {
        int dep_level = compute_module_level(dep, levels, visited);
        max_dep_level = std::max(max_dep_level, dep_level);
    }

    const int level = max_dep_level + 1;
    levels[module] = level;
    visited.erase(module);

    return level;
}
} // portal
