//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/core/events/event.h"

namespace portal
{

// TODO: use concept instead?

/**
 * A call capable of handling events
 */
class EventHandler
{
public:
    virtual ~EventHandler() = default;

    virtual void on_event(Event& event) = 0;
};

}
