//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <thread>
#include <concurrentqueue/concurrentqueue.h>

#include "portal/core/concurrency/spin_lock.h"
#include "portal/engine/resources/gpu_context.h"
#include "portal/engine/resources/loader/loader_factory.h"
#include "portal/engine/strings/string_id.h"
#include "utils.h"
#include "portal/core/reference.h"
#include "portal/core/thread.h"
#include "portal/core/debug/profile.h"

namespace portal
{
namespace resources
{
    class ResourceDatabase;
}

class Resource;

class ResourceRegistry
{
public:
    void initialize(const std::shared_ptr<resources::GpuContext>& context, const std::shared_ptr<resources::ResourceDatabase>& database);
    void shutdown();

    template <typename T> requires std::is_base_of_v<Resource, T>
    Ref<T> get(const StringId id)
    {
        auto resource_type = resources::utils::to_resource_type<T>();
        auto&& resource = get(id, resource_type);
        return std::move(resource.template as<T>());
    }

    Ref<Resource> get(StringId id, ResourceType type);

    template <typename T> requires std::is_base_of_v<Resource, T>
    Ref<T> immediate_load(const StringId id)
    {
        auto resource_type = resources::utils::to_resource_type<T>();
        auto&& resource = immediate_load(id, resource_type);
        return std::move(resource.template as<T>());
    }
    Ref<Resource> immediate_load(StringId id, ResourceType type);

    void unload(StringId id);

private:
    void resource_load_loop(const std::stop_token& stoken);
    static void load_resource(Ref<Resource>& ref, const std::shared_ptr<resources::ResourceLoader>&);

    std::shared_ptr<resources::GpuContext> gpu_context;
    std::unordered_map<StringId, Ref<Resource>> resource_map;

    Thread asset_loader_thread;
    moodycamel::ConcurrentQueue<std::pair<StringId, std::shared_ptr<resources::ResourceLoader>>> asset_load_queue;

    std::shared_ptr<resources::ResourceDatabase> resource_database;
    resources::LoaderFactory loader_factory;
};

} // portal
