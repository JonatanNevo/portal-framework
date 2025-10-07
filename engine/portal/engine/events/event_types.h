//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <cstdint>

namespace portal
{

enum class EventType: uint8_t
{
    Unknown = 0,
    // Window Events
    WindowClose,
    WindowMinimize,
    WindowResize,
    WindowFocus,
    WindowLostFocus,
    WindowMoved,
    WindowTitleBarHit
};

enum class EventCategory: uint8_t
{
    Unknown = 0,
    Window
};
}
