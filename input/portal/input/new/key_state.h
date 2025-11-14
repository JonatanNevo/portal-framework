//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/core/glm.h>

#include <glaze/core/reflect.hpp>

#include "llvm/ADT/SmallVector.h"
#include "portal/input/new/input_types.h"

namespace portal::ng
{


/**
 * Details about a key's state and recent events
 */
struct KeyState
{
    /**
     * The most recent raw value related to this key.
     * Digital buttons values are either 0 or 1, only in the `x` field of the vec3
     * Analog buttons values are 0->1
     * Analog Axis values are -1->1
     */
    glm::vec3 raw_value;

    /**
     * The processed and final value of the key
     */
    glm::vec3 value;

    /**
     * The time between the last up->down or down->up transition.
     */
    float last_state_change_transition_time;

    /**
     * If the button is `down` right now.
     */
    bool down : 1;

    /**
     * If the button was `down` in the previous key process.
     */
    bool down_previous : 1;

    /**
     * If the button was consumed by some action.
     */
    bool consumed : 1;

    /**
     * Whether this key state was flushed this frame.
     */
    bool just_flushed : 1;

    /**
     * A bitmap of which paired axes has been sampled in this frame.
     */
    uint8_t pair_sampled_axes : 3;

    /**
     * The number of samples in `raw_value_accumulator`
     */
    uint32_t sample_count_accumulator;

    /**
     * An accumulation of raw values during a certain frame.
     */
    glm::vec3 raw_value_accumulator;

    constexpr static size_t input_event_count = glz::reflect<InputEventType>::keys.size();

    /**
     * How many of each event type have been received when this input was lased processed.
     */
    llvm::SmallVector<uint32_t> event_counts[input_event_count];

    /**
     * Used to accumulate events during the frame.
     */
    llvm::SmallVector<uint32_t> event_accumulator[input_event_count];
};
}
