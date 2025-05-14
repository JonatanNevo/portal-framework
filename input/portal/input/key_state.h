//
// Created by thejo on 5/4/2025.
//

#pragma once

#include <array>
#include <portal/core/glm.h>

#include "input_types.h"

namespace portal
{
struct KeyState
{
    /**
     * This is the most recent raw value reported by the device.
     * For digital buttons, 0 or 1.
     * For analog buttons, 0->1.
     * For axes, -1->1.
     * The X field is for non-vector keys
     */
    glm::vec3 raw_value;

    /** The final "value" for this control, after any optional processing. */
    glm::vec3 value;

    /** Global time of last up->down or down->up transition. */
    float last_up_down_transition_time;

    /** True if this key is "down", false otherwise. */
    bool is_down : 1;

    /** Queued state information.  This data is updated or flushed once player input is processed. */
    bool is_down_previous : 1;

    /** True if this key has been "consumed" by an InputComponent and should be ignored for further components during this update. */
    bool is_consumed : 1;

    /**
    * True if this key was flushed this frame. This is used to flag to input processing that we may be receiving
    * a Repeat event, but the Pressed event accumulator may have been reset.
    */
    bool was_just_flushed : 1;

    /** Flag paired axes that have been sampled this tick. X = LSB, Z = MSB */
    uint8_t pair_sampled_axes : 3;

    /** How many samples contributed to raw_value_accumulator. Used for smoothing operations, e.g. mouse */
    uint32_t sample_count_accumulator;

    /** Used to accumulate input values during the frame and flushed after processing. */
    glm::vec3 raw_value_accumulator;

    /** How many of each event type had been received when input was last processed. */
    std::vector<uint32_t> event_counts[static_cast<uint8_t>(InputEvent::Max)];

    /** Used to accumulate events during the frame and flushed when processed. */
    std::vector<uint32_t> event_accumulator[static_cast<uint8_t>(InputEvent::Max)];

    KeyState() :
        raw_value(0.f, 0.f, 0.f)
        , value(0.f, 0.f, 0.f)
        , last_up_down_transition_time(0.f)
        , is_down(false)
        , is_down_previous(false)
        , is_consumed(false)
        , was_just_flushed(false)
        , pair_sampled_axes(0)
        , sample_count_accumulator(0)
        , raw_value_accumulator(0.f, 0.f, 0.f)
        , event_counts()
        , event_accumulator()
    {
    }
};
}
