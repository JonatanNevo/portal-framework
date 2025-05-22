//
// Created by thejo on 4/15/2025.
//

#include "input.h"

#include <execution>
#include <memory>
#include <ranges>

#include "portal/core/log.h"
#include "portal/core/random.h"

// TODO: decuple from GLFW

namespace portal
{
void key_callback(GLFWwindow* window, int key, int scan_code, int action, int /*mods*/)
{
    if (auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window)))
    {
        InputKeyParams params;
        params.key = InputKeyManager::get().get_key_from_codes(scan_code);
        if (!params.key.is_valid())
        {
            LOG_CORE_ERROR_TAG("Input", "Key {} is not valid!", key);
            return;
        }

        if (action == GLFW_PRESS)
            params.event = InputEvent::Pressed;
        if (action == GLFW_RELEASE)
            params.event = InputEvent::Released;
        if (action == GLFW_REPEAT)
            params.event = InputEvent::Repeat;

        input->input_key(params);
    }
}

void cursor_position_callback(GLFWwindow* window, const double xpos, const double ypos)
{
    if (auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window)))
    {
        InputKeyParams mouse_x(Keys::MouseX, InputEvent::Axis, glm::vec3(static_cast<float>(xpos), 0.f, 0.f));
        input->input_key(mouse_x);

        InputKeyParams mouse_y(Keys::MouseY, InputEvent::Axis, glm::vec3(static_cast<float>(ypos), 0.f, 0.f));
        input->input_key(mouse_y);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int /*mods*/)
{
    if (auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window)))
    {
        InputKeyParams params;
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            params.key = Keys::LeftMouseButton;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            params.key = Keys::RightMouseButton;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            params.key = Keys::MiddleMouseButton;
        if (button == GLFW_MOUSE_BUTTON_4)
            params.key = Keys::ThumbMouseButton;
        if (button == GLFW_MOUSE_BUTTON_5)
            params.key = Keys::ThumbMouseButton2;

        if (action == GLFW_PRESS)
            params.event = InputEvent::Pressed;
        if (action == GLFW_RELEASE)
            params.event = InputEvent::Released;
        if (action == GLFW_REPEAT)
            params.event = InputEvent::Repeat;

        input->input_key(params);
    }
}

Input::Input(GLFWwindow* window):
    window(window)
{
    // TODO: Create some "GLFW Application" or "GLFW Context" that will handle all glfw callbacks
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, 1);
}

bool Input::input_key(const InputKeyParams& params)
{
    // MouseX and MouseY should not be treated as analog if there are no samples, as they need their EventAccumulator to be incremented
    // in the case of a Pressed or Released
    const bool treat_as_analog =
        params.key.is_analog() &&
        ((params.key != Keys::MouseX && params.key != Keys::MouseX) || params.num_samples > 0);

    if (treat_as_analog)
    {
        auto test_event_edges = [this, &params](KeyState& key_state, float edge_value)
        {
            if (edge_value == 0.f && glm::length(params.delta) != 0.f)
            {
                key_state.event_accumulator[static_cast<uint8_t>(InputEvent::Pressed)].push_back(++event_count);
            }
            else if (edge_value != 0.f && glm::length(params.delta) == 0.f)
            {
                key_state.event_accumulator[static_cast<uint8_t>(InputEvent::Released)].push_back(++event_count);
            }
            else
            {
                key_state.event_accumulator[static_cast<uint8_t>(InputEvent::Repeat)].push_back(++event_count);
            }
        };

        {
            // first event associated with this key, add it to the map
            KeyState& state = key_state_map[params.key];
            test_event_edges(state, state.value.x);

            // accumulate deltas until processed next
            state.sample_count_accumulator += params.num_samples;
            state.raw_value_accumulator += params.delta;
        }

        // Mirror the key press to any associated paired axis
        Key paired_key = params.key.get_paired_axis_key();
        if (paired_key.is_valid())
        {
            PairedAxis axis = params.key.get_paired_axis();
            KeyState& paired_state = key_state_map[paired_key];

            KeyState& state = key_state_map[params.key];

            if (axis == PairedAxis::X)
            {
                paired_state.raw_value_accumulator.x = state.raw_value_accumulator.x;
                paired_state.pair_sampled_axes |= 0b001;
            }
            else if (axis == PairedAxis::Y)
            {
                paired_state.raw_value_accumulator.y = state.raw_value_accumulator.x;
                paired_state.pair_sampled_axes |= 0b010;
            }
            else if (axis == PairedAxis::Z)
            {
                paired_state.raw_value_accumulator.z = state.raw_value_accumulator.x;
                paired_state.pair_sampled_axes |= 0b100;
            }
            else
            {
                LOG_CORE_ERROR_TAG("Input", "Key {} has paired axis key {} but no valid paired axis!", params.key.get_name(), paired_key.get_name());
            }

            paired_state.sample_count_accumulator = std::max(paired_state.sample_count_accumulator, state.sample_count_accumulator);
            test_event_edges(paired_state, length(paired_state.value));
        }

        return false;
    }
    // Non-analog key
    else
    {
        // first event associated with this key, add it to the map
        KeyState& state = key_state_map[params.key];

        const float time_point = timer.elapsed();

        switch (params.event)
        {
        case InputEvent::Pressed:
        case InputEvent::Repeat:
            state.raw_value_accumulator.x = params.delta.x;
            state.event_accumulator[static_cast<uint8_t>(params.event)].push_back(++event_count);
            if (!state.is_down_previous)
            {
                // check for doubleclick
                // note, a tripleclick will currently count as a 2nd double click.
                constexpr float double_click_time = 0.2f; // TODO: Get this from some config
                if (time_point - state.last_up_down_transition_time < double_click_time)
                {
                    state.event_accumulator[static_cast<uint8_t>(InputEvent::DoubleClick)].push_back(++event_count);
                }

                // just went down
                state.last_up_down_transition_time = time_point;
            }
            break;
        case InputEvent::Released:
            state.raw_value_accumulator.x = 0.f;
            state.event_accumulator[static_cast<uint8_t>(InputEvent::Released)].push_back(++event_count);
            break;
        case InputEvent::DoubleClick:
            state.raw_value_accumulator.x = params.delta.x;
            state.event_accumulator[static_cast<uint8_t>(InputEvent::Pressed)].push_back(++event_count);
            state.event_accumulator[static_cast<uint8_t>(InputEvent::DoubleClick)].push_back(++event_count);
            break;
        default:
            break;
        }
        state.sample_count_accumulator++;

        // We have now processed this key's state, so we can clear its "just flushed" flag and treat it normally
        state.was_just_flushed = false;

        return true;
    }
}

void Input::process_inputs(const float delta_time)
{
    static std::unordered_map<Key, KeyState*> keys_with_events;
    evaluate_key_states(delta_time, keys_with_events);
    evaluate_input_delegates(delta_time, keys_with_events);
    finish_processing_inputs();
    keys_with_events.clear();
}

input::ActionBinding& Input::add_action_binding(input::ActionBinding in_bind)
{
    const auto& binding = action_bindings.emplace_back(std::make_shared<input::ActionBinding>(std::move(in_bind)));
    binding->generate_new_handle();

    if (binding->event == InputEvent::Pressed || binding->event == InputEvent::Released)
    {
        const auto paired_event = binding->event == InputEvent::Pressed ? InputEvent::Released : InputEvent::Pressed;
        for (int32_t i = static_cast<int32_t>(action_bindings.size()) - 2; i >= 0; i--)
        {
            auto& action_binding = action_bindings[i];
            if (action_binding->get_key() == binding->get_key())
            {
                // If we find a matching event that is already paired we know this is paired so mark it off and we're done
                if (action_binding->is_paired())
                {
                    binding->paired = true;
                    break;
                }

                // Otherwise if this is a pair to the new one mark them both as paired
                // Don't break as there could be two bound paired events
                if (action_binding->event == paired_event)
                {
                    action_binding->paired = true;
                    binding->paired = true;
                }
            }
        }
    }

    return *binding;
}

void Input::clear_action_binding()
{
    action_bindings.clear();
}

void Input::evaluate_key_states(const float delta_time, std::unordered_map<Key, KeyState*>& keys_with_events)
{
    for (auto& [key, state] : key_state_map)
    {
        bool key_has_events = false;

        for (size_t i = 0; i < static_cast<uint8_t>(InputEvent::Max); i++)
        {
            state.event_counts[i].clear();
            std::swap(state.event_counts[i], state.event_accumulator[i]);

            if (!key_has_events && state.event_counts[i].size() > 0)
            {
                keys_with_events[key] = &state;
                key_has_events = true;
            }
        }

        if (state.sample_count_accumulator > 0 || key.should_update_axis_without_samples())
        {
            if (state.pair_sampled_axes)
            {
                // Paired keys sample only the axes that have changed, leaving unaltered axes in their previous state
                for (int axis = 0; axis < 3; ++axis)
                {
                    if (state.pair_sampled_axes & (1 << axis))
                    {
                        state.raw_value[axis] = state.raw_value_accumulator[axis];
                    }
                }
            }
            else
            {
                // Unpaired keys just take the whole accumulator
                state.raw_value = state.raw_value_accumulator;
            }

            // if we had no samples, we'll assume the state hasn't changed
            // except for some axes, where no samples means the mouse stopped moving
            if (state.sample_count_accumulator == 0)
            {
                state.event_counts[static_cast<uint8_t>(InputEvent::Released)].push_back(++event_count);
                if (!key_has_events)
                {
                    keys_with_events[key] = &state;
                    key_has_events = true;
                }
            }
        }

        if (key == Keys::MouseX && state.raw_value_accumulator.x != 0.f)
        {
            // calculate sampling time
            // make sure not first non-zero sample
            if (smoothed_mouse[0] != 0)
            {
                // not first non-zero
                mouse_sampling_total += delta_time;
                mouse_samples += state.sample_count_accumulator;
            }
        }

        // if (key.is_axis_2D())
        // {
        // state.value = MassageVectorAxisInput(InKey, KeyState->RawValue);
        // }
        // else
        // {
        // KeyState->Value.X = MassageAxisInput(InKey, KeyState->RawValue.X);
        // }

        const auto press_delta = state.event_counts[static_cast<uint8_t>(InputEvent::Pressed)].size() - state.event_counts[static_cast<uint8_t>(
            InputEvent::Released)].size();
        if (press_delta < 0)
        {
            // If this is negative, we definitely released
            state.is_down = false;
        }
        else if (press_delta > 0)
        {
            // If this is positive, we definitely pressed
            state.is_down = true;
        }
        else
        {
            // If this is 0, we maintain state
            state.is_down = state.is_down_previous;
        }

        // reset the accumulators
        state.raw_value_accumulator = glm::vec3(0);
        state.sample_count_accumulator = 0;
        state.pair_sampled_axes = 0;
    }
    event_count = 0;
}

void Input::evaluate_input_delegates(float /*delta_time*/, std::unordered_map<Key, KeyState*>& keys_with_events)
{
    // TODO: Handle axis
    static std::vector<std::shared_ptr<input::ActionBinding>> actions;

    for (auto& [key, state] : keys_with_events)
    {
        // TODO: Use map instead
        for (auto& action_bind: action_bindings)
        {
            if (action_bind->get_key() == key)
            {
                if (!state->event_counts[static_cast<uint8_t>(action_bind->event)].empty())
                {
                    actions.emplace_back(action_bind);
                }
            }
        }

        for (auto& action: actions)
        {
            if (action->delegate.is_bound())
            {
                // TODO: Cache all delegates and fire simultaneously
                action->delegate.execute(key);
            }
        }
    }

    actions.clear();
}

void Input::finish_processing_inputs()
{
    // finished processing input for this frame, clean up for next update
    for (auto& state : key_state_map | std::views::values)
    {
        state.is_down_previous = state.is_down;
        state.is_consumed = false;
    }
}
} // portal
