//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

namespace portal
{

struct EditorContext;
struct FrameContext;

class Panel
{
public:
    virtual ~Panel() = default;

    virtual void on_gui_render(EditorContext& editor_context, FrameContext& frame_context) = 0;
    // TODO: add on event?
};
}