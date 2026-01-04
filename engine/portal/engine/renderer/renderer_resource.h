//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/resources/resources/resource.h"

namespace portal::renderer
{
/**
 * @class RendererResource
 * @brief Base class for renderer resources
 *
 * Extends Resource for renderer-specific resource types.
 */
class RendererResource : public Resource
{
public:
    /**
     * @brief Constructs renderer resource
     * @param id Resource identifier
     */
    explicit RendererResource(const StringId& id) : Resource(id) {}
};
}
