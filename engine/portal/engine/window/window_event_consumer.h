//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/window/window.h"

namespace portal
{

/**
 * An interface for a class that consumes window events (resize, focus, request close, etc...)
 */
class WindowEventConsumer
{
public:
    virtual ~WindowEventConsumer() = default;

    virtual void on_resize(WindowExtent extent) = 0;
    virtual void on_focus(bool focused) = 0;
    virtual void on_close() = 0;
};

}
