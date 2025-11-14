//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "llvm/ADT/DenseMap.h"
#include "portal/core/timer.h"
#include "portal/input/new/input_types.h"
#include "portal/input/new/key_state.h"

namespace portal::ng
{
struct ActionKeyMapping;
}

namespace portal::ng
{

struct InputKeyEventParams
{
    // TODO: add some `input device id` to represent which device sent this input

    /**
     * The key that triggered this input event
     */
    Key key = Key::Invalid;

    /**
     * The type of event that occurred
     */
    InputEventType event = InputEventType::Pressed;

    /**
     * Delta time between that last frame and this frame
     */
    float delta_time = 1 / 60.f;

    /**
     * For analog key events,
     * The number of samples that the value has
     */
    int32_t num_samples = 1;

    /**
     * The value that this input event has.
     */
    float amount_pressed = 0;
};

class Input
{
public:
    Input();

    /**
     * Processes all "in flight" input events and calls the corresponding action callbacks
     */
    void process_inputs(float delta_time);

    /**
     * Flushes the lingering key states. used when transitioning input modes.
     */
    void flush_key_states();

    /**
     * Handles a key input event.
     * @return `true` if there is a registered action that handled the event, false otherwise.
     */
    bool input_key(const InputKeyEventParams& params);

    /**
     * @return true if the alt key is pressed
     */
    [[nodiscard]] bool is_alt_pressed() const;
    /**
     *@return true if the ctrl key is pressed
     */
    [[nodiscard]] bool is_ctrl_pressed() const;
    /**
     * @return true if the shift key is pressed
     */
    [[nodiscard]] bool is_shift_pressed() const;
    /**
     * @return true if the system key is pressed
     */
    [[nodiscard]] bool is_system_pressed() const;
    /**
     * @return True if the given key is currently pressed
     */
    [[nodiscard]] bool is_pressed(const Key& key) const;

private:
    bool process_analog_event(const InputKeyEventParams& params);
    bool process_digital_event(const InputKeyEventParams& params);

    bool is_key_handled_by_action(const Key& key) const;

    void evaluate_key_states(float delta_time, llvm::SmallVectorImpl<std::pair<Key, KeyState*>>& keys_with_events);
    void process_non_axes_keys(Key key, KeyState& state);

private:
    /**
     * A counter use to track the order in which events have occurred since the last input process time
     */
    uint32_t event_count = 0;

    llvm::DenseMap<Key, KeyState> key_state_map;

    llvm::SmallVector<ActionKeyMapping> action_mappings;

    /**
     * A timer that counts from the beginning of the input system
     */
    // TODO: have one global `application` timer
    Timer timer;
};

} // portal
