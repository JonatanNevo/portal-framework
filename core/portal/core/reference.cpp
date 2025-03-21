//
// Created by Jonatan Nevo on 21/03/2025.
//

#include "reference.h"

#include <mutex>
#include <unordered_set>

#include "portal/core/assert.h"

namespace portal
{

static std::unordered_set<void*> s_live_references;
static std::mutex s_live_references_mutex;

void reference::add_to_live_reference(void* instance)
{
    std::scoped_lock lock(s_live_references_mutex);
    PORTAL_CORE_ASSERT(instance, "Instance is invalid");
    s_live_references.insert(instance);
}

void reference::remove_from_live_reference(void* instance)
{
    std::scoped_lock lock(s_live_references_mutex);
    PORTAL_CORE_ASSERT(instance, "Instance is invalid");
    PORTAL_CORE_ASSERT(s_live_references.contains(instance), "Instance already removed");
    s_live_references.erase(instance);
}

bool reference::is_live(void* instance)
{
    PORTAL_CORE_ASSERT(instance, "Instance is invalid");
    return s_live_references.contains(instance);
}

void CountedReference::inc_ref_count() const
{
    ++ref_count;
}

void CountedReference::dec_ref_count() const
{
    --ref_count;
}

uint32_t CountedReference::get_ref_count() const
{
    return ref_count.load();
}
}
