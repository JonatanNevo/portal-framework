//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "reference_manager.h"

#include "portal/engine/resources/new/resource_reference.h"

namespace portal::ng
{

static auto logger = Log::get_logger("Resources");

ReferenceManager::~ReferenceManager()
{
    if (!references.empty())
        LOG_ERROR("Reference manager destroyed with {} references still registered", references.size());
}

void ReferenceManager::register_reference(const ResourceHandle handle, void* reference)
{
    references[handle].insert(reference);
}

void ReferenceManager::unregister_reference(const ResourceHandle handle, void* reference)
{
    if (!references.contains(handle))
    {
        LOG_WARN("Attempted to unregister reference for resource handle {} that does not exist", handle);
        return;
    }
    if (!references.at(handle).contains(reference))
    {
        LOG_WARN("Attempted to unregister reference for resource handle {} that does not exist", handle);
        return;
    }

    references[handle].erase(reference);
}

void ReferenceManager::move_reference(const ResourceHandle handle, void* old_ref, void* new_ref)
{
    unregister_reference(handle, old_ref);
    register_reference(handle, new_ref);
}

} // portal