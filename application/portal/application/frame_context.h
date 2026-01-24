//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

/**
 * @file frame_context.h
 * @brief Per-frame data structures passed through module lifecycle hooks.
 *
 * This file defines the data structures that carry frame-specific state through
 * the Application's main loop and into each module's lifecycle methods. These
 * structures enable modules to access shared frame state without tight coupling.
 */

#pragma once

#include <any>

namespace portal
{
class Scene;

namespace ecs
{
    class Registry;
}

/**
 * Performance statistics accumulated during a single frame.
 *
 * FrameStats tracks rendering and update performance metrics that are collected
 * throughout the frame's execution. Modules can read these statistics to make
 * performance-based decisions or write to them to report their own contributions.
 * The statistics are reset at the beginning of each frame.
 */
struct FrameStats
{
    float frame_time = 0.0001f;
    int triangle_count;
    int drawcall_count;
    float scene_update_time;
    float mesh_draw_time;
};


/**
 * Per-frame context data passed to all module lifecycle methods.
 *
 * FrameContext is the primary data structure that flows through the Application's
 * main loop, carrying essential per-frame state to every module's lifecycle hooks
 * (begin_frame, update, gui_update, post_update, end_frame). This structure enables
 * modules to access shared frame state and resources without direct dependencies
 * on each other.
 *
 * The same FrameContext instance is passed through an entire frame's execution,
 * allowing modules to communicate through the stats field or share the rendering
 * context. The frame_index wraps around based on frames_in_flight (typically 3)
 * to support multi-buffering in the renderer.
 *
 * Lifecycle flow:
 * 1. Application creates a new FrameContext at the start of each frame
 * 2. Passes it to modules.begin_frame(context)
 * 3. Passes it through modules.update(context)
 * 4. Passes it through modules.gui_update(context)
 * 5. Passes it through modules.post_update(context) for rendering
 * 6. Passes it to modules.end_frame(context) for cleanup
 *
 */
struct FrameContext
{
    size_t frame_index;
    float delta_time;
    FrameStats stats = {};
    // TODO: have in `ecs_context` instead of in global context?
    ecs::Registry* ecs_registry = nullptr;

    // When rendering_context is set, it should be a `renderer::RenderingContext`
    // TODO: this might cause performance issues, especially since any can use dynamic allocations without custom allocators, investigate
    std::any rendering_context = std::any{};

    // When scene_context is set, it should be a `SceneContext`
    std::any scene_context = std::any{};


    // TODO: add a custom stack allocator that will handle all of the frame's allocations
};
}
