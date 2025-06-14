//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <thread>
#include <concurrentqueue/concurrentqueue.h>

#include "portal/core/concurrency/spin_lock.h"
#include "portal/engine/strings/string_id.h"

namespace portal
{
namespace resources
{
    class ResourceDatabase;
}

class Resource;

// TODO: is this the correct approach? maybe use raii instead
struct ResourceReference
{
public:


private:
    friend class ResourceRegistry;

    size_t ref_count{0};
    std::shared_ptr<Resource> resource;
    SpinLock lock;

    void increment_ref();
    void decrement_ref();
};

class ResourceRegistry
{
public:
    template <typename T> requires std::is_base_of_v<Resource, T>
    std::shared_ptr<T> get(const StringId id)
    {
        auto&& resource = get(id);
        return std::move(std::dynamic_pointer_cast<T>(resource));
    }

    std::shared_ptr<Resource> get(StringId id);

    void unload(StringId id);

private:
    void load_assets(const std::stop_token& stoken);

    std::jthread asset_loader_thread;
    moodycamel::ConcurrentQueue<StringId> asset_load_queue;
    std::unique_ptr<resources::ResourceDatabase> database;

    std::unordered_map<StringId, ResourceReference> resources;
};

} // portal
