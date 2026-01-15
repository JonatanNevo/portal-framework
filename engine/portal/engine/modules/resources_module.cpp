//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "resources_module.h"

#include "portal/application/settings.h"
#include "portal/engine/resources/reference_manager.h"
#include "portal/engine/resources/resource_registry.h"
#include "portal/engine/resources/database/resource_database_facade.h"

namespace portal
{
ResourcesModule::ResourcesModule(ModuleStack& stack, renderer::vulkan::VulkanContext& context) : TaggedModule(stack, STRING_ID("Resources Module"))
{
    auto& settings = Settings::get();

    database = std::make_unique<ResourceDatabaseFacade>();
    if (settings.get_setting<bool>("engine.include_engine_resources", true))
    {
        database->register_database({DatabaseType::Folder, "engine"});
    }

    const auto descriptions = settings.get_setting<std::vector<DatabaseDescription>>("engine.resources");
    for (auto& description : descriptions.value_or(std::vector<DatabaseDescription>{}))
    {
        database->register_database(description);
    }

    reference_manager = std::make_unique<ReferenceManager>();

    registry = std::make_unique<ResourceRegistry>(
        get_dependency<SystemOrchestrator>(),
        get_dependency<SchedulerModule>().get_scheduler(),
        *database,
        *reference_manager,
        context
    );
}

ResourcesModule::~ResourcesModule() = default;
} // portal
