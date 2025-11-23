//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <any>

namespace portal
{

struct FrameStats
{
    float frame_time = 0.0001f;
    int triangle_count;
    int drawcall_count;
    float scene_update_time;
    float mesh_draw_time;
};


struct FrameContext
{
    size_t frame_index;
    float delta_time;
    FrameStats stats = {};

    // When rendering_context is set, it should be a `renderer::RenderingContext`
    // TODO: this might cause performance issues, especially since any can use dynamic allocations without custom allocators, investigate
    std::any rendering_context;

    // TODO: add a custom stack allocator that will handle all of the frame's allocations
};

}