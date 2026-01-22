//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "system_orchestrator.h"
#include "portal/application/modules/module.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/engine/resources/database/resource_database_facade.h"

namespace portal
{
class ResourceRegistry;
class ReferenceManager;

/**
 * @brief Module responsible for managing engine resources.
 *
 * ResourcesModule owns and initializes the resource management infrastructure:
 * - ResourceDatabaseFacade for loading resources from disk
 * - ReferenceManager for tracking resource references
 * - ResourceRegistry for storing and accessing loaded resources
 *
 */
class ResourcesModule final: public Module<SchedulerModule, ecs::Registry>
{
public:
    /**
     * @brief Constructs the resources module and initializes the resource system.
     * @param stack The module stack this module belongs to.
     * @param context The Vulkan context for GPU resource creation.
     */
    ResourcesModule(ModuleStack& stack, Project& project, renderer::vulkan::VulkanContext& context);

    /**
     * @brief Gets the resource registry.
     * @return Reference to the resource registry.
     */
    [[nodiscard]] ResourceRegistry& get_registry() const { return *registry; }

private:
    std::unique_ptr<ReferenceManager> reference_manager;
    std::unique_ptr<ResourceRegistry> registry;
};

} // portal