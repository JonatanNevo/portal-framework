//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <deque>
#include <functional>


namespace portal
{
/**
 * @class DeletionQueue
 * @brief Deferred resource destruction queue with LIFO execution order
 *
 * Vulkan resources cannot be destroyed while GPU commands referencing them are in-flight.
 * DeletionQueue solves this by deferring destruction until a safe point (e.g., frame completion).
 *
 * ## LIFO Execution Order
 *
 * Deleters execute in **reverse order** (last-in-first-out) during flush(). This respects
 * Vulkan's dependency constraints: if you create a buffer then an image view of that buffer,
 * the image view must be destroyed before the buffer.
 *
 * Usage:
 * @code
 * // Per-frame deletion queue
 * DeletionQueue frame_cleanup;
 *
 * // Instead of destroying immediately:
 * frame_cleanup.push_deleter([buffer = std::move(buffer)]() mutable {
 *     buffer = nullptr;  // Triggers destructor when safe
 * });
 *
 * // Later, when frame completes:
 * frame_cleanup.flush();  // Executes deleters in reverse order
 * @endcode
 */
class DeletionQueue
{
public:
    /**
     * @brief Adds deleter lambda to queue
     * @param deleter Lambda to execute during flush (captures resources to destroy)
     */
    void push_deleter(std::function<void()>&& deleter);

    /**
     * @brief Executes all deleters in reverse order (LIFO) and clears queue
     */
    void flush();

private:
    std::deque<std::function<void()>> deletion_queue;
};
} // portal
