//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

namespace portal::renderer
{
/**
 * @class Queue
 * @brief Abstract interface for command queue
 *
 * Minimal queue abstraction providing queue index access.
 * Concrete implementations (VulkanQueue) provide submit and present operations.
 */
class Queue
{
public:
    virtual ~Queue() = default;

    /**
     * @brief Gets queue index within its family
     * @return Queue index
     */
    [[nodiscard]] virtual size_t get_index() const = 0;

    // TODO: add `submit` `present`
};
}
