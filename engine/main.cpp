//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <nlohmann/json.hpp>

#include "portal/core/log.h"
#include "portal/engine/renderer/renderer.h"
#include "portal/engine/resources/resource_registry.h"
#include "portal/engine/resources/database/folder_resource_database.h"

int main()
{
    portal::Log::init({.default_log_level = portal::Log::LogLevel::Trace});
    try
    {
        portal::Renderer renderer;
        renderer.init();

        auto resource_database = std::make_shared<portal::FolderResourceDatabase>("C:/Code/portal-framework/engine/shaders");
        auto registry = portal::ResourceRegistry();
        registry.initialize(renderer.get_gpu_context(), resource_database);

        auto shader = registry.immediate_load<portal::Shader>(STRING_ID("PBR/pbr.slang"));;
        // {
        //     registry.immediate_load<portal::Scene>(STRING_ID("ABeautifulGame.gltf"));
        //     auto scene = registry.get<portal::Scene>(STRING_ID("Scene0-Scene"));
        //     renderer.set_scene(scene);
        //     renderer.run();
        // }

        registry.shutdown();
        renderer.cleanup();
    }
    catch (const std::exception& e)
    {
        LOG_FATAL("Exception caught: {}", e.what());
    }
    catch (...)
    {
        LOG_FATAL("Fatal unknown exception caught");
    }
    portal::Log::shutdown();
}
