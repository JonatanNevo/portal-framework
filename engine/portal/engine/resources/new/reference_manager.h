//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallSet.h"

#include "portal/engine/resources/new/resource.h"

namespace portal::ng
{

class ReferenceManager
{
public:
    ~ReferenceManager();

    /**
     * Registers a new reference for the reference counting
     * @note we are using void* here because `ResourceReference` is a template function, and we don't need to access it directly from the registry
     *
     * @param handle the handle to the resource in the registry
     * @param reference a void* pointer to the reference.
     */
    void register_reference(ResourceHandle handle, void* reference);

    /**
     * Removes a reference from the reference counting
     *
    * @param handle the handle to the resource in the registry
     * @param reference a void* pointer to the reference.
     */
    void unregister_reference(ResourceHandle handle, void* reference);

    /**
     * Switches between two references in the reference counting, the same as calling `unregister(old) register(new)` but makes sure that there is always
     * a valid reference (used in the ResourceReference move operators)
     *
     * @param handle The resource handle
     * @param old_ref The old reference (as a void*)
     * @param new_ref The new reference (as a void*)
     */
    void move_reference(ResourceHandle handle, void* old_ref, void* new_ref);

private:
    llvm::DenseMap<ResourceHandle, llvm::SmallSet<void*, 16>> references;
};
} // portal