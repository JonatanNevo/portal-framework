//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "reference_manager.h"

#include "resource_reference.h"

namespace portal
{

static auto logger = Log::get_logger("Resources");

ReferenceManager::~ReferenceManager()
{
    bool ok = true;
    for (auto& [ref, set] : references)
    {
        if (!set.empty())
        {
            LOG_ERROR("Reference manager destroyed with {} references still registered for reference: {}", set.size(), ref);
            ok = false;
        }
    }

    PORTAL_ASSERT(ok, "Reference manager destroyed with references still registered");
}

void ReferenceManager::register_reference(const StringId& id, void* reference)
{
    std::lock_guard guard(lock);
    references[id].insert(reference);
}

void ReferenceManager::unregister_reference(const StringId& id, void* reference)
{
    std::lock_guard guard(lock);
    if (!references.contains(id))
    {
        LOG_WARN("Attempted to unregister reference for resource handle {} that does not exist", id);
        return;
    }
    if (!references.at(id).contains(reference))
    {
        LOG_WARN("Attempted to unregister reference for resource handle {} that does not exist", id);
        return;
    }

    references[id].erase(reference);
}

void ReferenceManager::move_reference(const StringId& id, void* old_ref, void* new_ref)
{
    std::lock_guard guard(lock);
    unregister_reference(id, old_ref);
    register_reference(id, new_ref);
}

} // portal