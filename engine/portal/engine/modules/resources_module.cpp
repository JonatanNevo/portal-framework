//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "resources_module.h"

#include "portal/application/settings.h"
#include "portal/engine/project/project.h"
#include "portal/engine/resources/reference_manager.h"
#include "portal/engine/resources/resource_registry.h"
#include "portal/engine/resources/database/resource_database_facade.h"

namespace portal
{
ResourcesModule::ResourcesModule(ModuleStack& stack, Project& project, renderer::vulkan::VulkanContext& context)
    : TaggedModule(
        stack,
        STRING_ID("Resources Module")
    )
{
    reference_manager = std::make_unique<ReferenceManager>();

    registry = std::make_unique<ResourceRegistry>(
        project,
        get_dependency<ecs::Registry>(),
        get_dependency<SchedulerModule>().get_scheduler(),
        project.get_resource_database(),
        *reference_manager,
        context
    );
}
} // portal
