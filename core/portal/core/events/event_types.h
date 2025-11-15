//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <cstdint>
#include <portal/core/flags.h>

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
    WindowTitleBarHit,
    // Input Events
    KeyPressed,
    KeyReleased,
    KeyRepeat,
    MouseMoved,
    MouseScrolled,
    SetMouseCursor
};

enum class EventCategoryBits: uint8_t
{
    Unknown = 0,
    Window,
    Input
};

using EventCategory = Flags<EventCategoryBits>;

template<>
struct FlagTraits<EventCategoryBits>
{
    using enum EventCategoryBits;

    static constexpr bool is_bitmask = true;
    static constexpr auto all_flags = Window | Input;
};

}


