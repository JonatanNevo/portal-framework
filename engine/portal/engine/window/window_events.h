//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/window/window.h"

namespace portal
{

struct WindowResizeEvent
{
    WindowExtent extent;
};

struct WindowFocusEvent
{
    bool focused;
};

struct WindowClosedEvent {};

struct WindowRequestMinimizeEvent{};
struct WindowRequestMaximizeOrRestoreEvent{};
struct WindowRequestCloseEvent{};


struct WindowDragEvent
{
    float original_window_width;
    glm::vec2 point;
    glm::vec2 move_offset;
};

}