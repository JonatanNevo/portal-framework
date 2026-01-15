//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <unordered_set>

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallSet.h"
#include "portal/application/modules/module.h"
#include "portal/core/concurrency/reentrant_spin_lock.h"
#include "portal/core/concurrency/spin_lock.h"

#include "resources/resource.h"

namespace portal
{
class ReferenceManager
{
public:
    ~ReferenceManager();

    /**
     * Registers a new reference for the reference counting
     * @note we are using void* here because `ResourceReference` is a template function, and we don't need to access it directly from the registry
     *
     * @param id the handle to the resource in the registry
     * @param reference a void* pointer to the reference.
     */
    void register_reference(const StringId& id, void* reference);

    /**
     * Removes a reference from the reference counting
     *
    * @param id the handle to the resource in the registry
     * @param reference a void* pointer to the reference.
     */
    void unregister_reference(const StringId& id, void* reference);

    /**
     * Switches between two references in the reference counting, the same as calling `unregister(old) register(new)` but makes sure that there is always
     * a valid reference (used in the ResourceReference move operators)
     *
     * @param id The resource handle
     * @param old_ref The old reference (as a void*)
     * @param new_ref The new reference (as a void*)
     */
    void move_reference(const StringId& id, void* old_ref, void* new_ref);

private:
    ReentrantSpinLock<> lock;
#ifdef PORTAL_DEBUG
    std::unordered_map<StringId, std::unordered_set<void*>> references;
#else
    llvm::DenseMap<StringId, llvm::SmallSet<void*, 16>> references;
#endif
};
} // portal
