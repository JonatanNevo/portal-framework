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

static std::unordered_set<ref_utils::LiveReference> live_references;
static SpinLock life_ref_lock;

constexpr bool ref_utils::LiveReference::operator==(const LiveReference& other) const
{
    return other.instance == instance;
}

void ref_utils::add_to_live(const LiveReference& instance)
{
    PORTAL_ASSERT(instance.instance, "Attempting to reference a null ptr.");

    std::lock_guard guard(life_ref_lock);
    live_references.insert(instance);
}

void ref_utils::remove_from_live(const LiveReference& instance)
{
    PORTAL_ASSERT(instance.instance, "Attempting to remove a null ptr.");
    // PORTAL_ASSERT(live_references.contains(instance), "Attempting to remove a reference that is not live.");

    instance.destructor(instance.instance);

    std::lock_guard guard(life_ref_lock);
    live_references.erase(instance);
}

bool ref_utils::is_live(const LiveReference& instance)
{
    return live_references.contains(instance);
}

void ref_utils::clean_all_references()
{
    const auto lock_result = life_ref_lock.try_lock();
    PORTAL_ASSERT(lock_result, "Attempting to clean references while another thread is using the ref counter.");
    PORTAL_ASSERT(live_references.size() == 0, "Attempting to clean references while there are still live references.");

    for (auto ref : live_references)
        ref.destructor(ref.instance);

    live_references.clear();

    if (lock_result)
        life_ref_lock.unlock();
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
