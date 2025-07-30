//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "reference.h"

#include <mutex>
#include <unordered_set>

#include "portal/core/concurrency/spin_lock.h"
#include "portal/core/debug/assert.h"

namespace portal
{

static std::unordered_set<void*> live_references;
static SpinLock life_ref_lock;

void ref_utils::add_to_live(void* instance)
{
    PORTAL_ASSERT(instance, "Attempting to reference a null ptr.");

    std::lock_guard guard(life_ref_lock);
    live_references.insert(instance);
}

void ref_utils::remove_from_live(void* instance)
{
    PORTAL_ASSERT(instance, "Attempting to remove a null ptr.");
    PORTAL_ASSERT(live_references.contains(instance), "Attempting to remove a reference that is not live.");

    std::lock_guard guard(life_ref_lock);
    live_references.erase(instance);
}

bool ref_utils::is_live(void* instance)
{
    return live_references.contains(instance);
}

void RefCounted::inc_ref() const
{
    ++ref_count;
}

void RefCounted::dec_ref() const
{
    --ref_count;
}

size_t RefCounted::get_ref() const
{
    return ref_count.load();
}
} // portal
