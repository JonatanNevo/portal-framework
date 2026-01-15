//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "system_orchestrator.h"
#include "portal/application/modules/module.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"

namespace portal
{
class ResourceDatabaseFacade;
class ResourceRegistry;
class ReferenceManager;


class ResourcesModule final: public Module<SchedulerModule, SystemOrchestrator>
{
public:
    ResourcesModule(ModuleStack& stack, renderer::vulkan::VulkanContext& context);

    [[nodiscard]] ResourceRegistry& get_registry() const { return *registry; }

private:
    std::unique_ptr<ResourceDatabaseFacade> database;
    std::unique_ptr<ReferenceManager> reference_manager;
    std::unique_ptr<ResourceRegistry> registry;
};

} // portal