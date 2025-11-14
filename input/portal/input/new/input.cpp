//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "input.h"

#include "portal/input/new/action_mappings.h"
#include "portal/input/new/key_state.h"
#include "portal/input/new/key_mapping.h"

namespace portal::ng
{

static auto logger = Log::get_logger("Input");

Input::Input()
{
    timer.start();
}

void Input::process_inputs(float delta_time)
{
    static llvm::SmallVector<std::pair<Key, KeyState*>> keys_with_events;
}

void Input::flush_key_states()
{
    LOGGER_DEBUG("Flushing key states");

    const auto elapsed = timer.elapsed<Timer::Seconds>();
    for (auto& [key, state] : key_state_map)
    {
        state.raw_value = glm::vec3{0};
        state.down = false;
        state.down_previous = false;
        state.last_state_change_transition_time = elapsed;
        state.just_flushed = true;
    }
}

bool Input::input_key(const InputKeyEventParams& params)
{
    auto& details = KeyMapping::get_details(params.key);

    // MouseX and MouseY are not treated as analog if they have no samples
    const bool treat_as_analog = details.is_analog() && ((params.key != Key::MouseX && params.key != Key::MouseY) || params.num_samples > 0);

    if (treat_as_analog)
        return process_analog_event(params);
    return process_digital_event(params);
}

bool Input::is_alt_pressed() const
{
    return is_pressed(Key::LeftAlt) || is_pressed(Key::RightAlt);
}

bool Input::is_ctrl_pressed() const
{
    return is_pressed(Key::LeftControl) || is_pressed(Key::RightControl);
}

bool Input::is_shift_pressed() const
{
    return is_pressed(Key::LeftShift) || is_pressed(Key::RightShift);
}

bool Input::is_system_pressed() const
{
    return is_pressed(Key::LeftSystem) || is_pressed(Key::RightSystem);
}

bool Input::is_pressed(const Key& key) const
{
    if (key == Key::Any)
    {
        return std::ranges::any_of(
            key_state_map,
            [](const auto& pair)
            {
                auto& [key, value] = pair;
                auto& details = KeyMapping::get_details(key);

                return details.is_digital() && value.down;
            }
            );
    }

    const auto it = key_state_map.find(key);
    if (it == key_state_map.end())
        return false;

    return it->second.down;
}

bool Input::process_analog_event(const InputKeyEventParams& params)
{
    auto& details = KeyMapping::get_details(params.key);

    auto test_event_edge_values = [this, &params](KeyState& state, const float edge_value)
    {
        if (edge_value == 0.f && params.amount_pressed != 0.f)
            state.event_accumulator[static_cast<uint8_t>(InputEventType::Pressed)].emplace_back(++event_count);
        else if (edge_value != 0.f && params.amount_pressed == 0.f)
            state.event_accumulator[static_cast<uint8_t>(InputEventType::Released)].emplace_back(++event_count);
        else
            state.event_accumulator[static_cast<uint8_t>(InputEventType::Repeat)].emplace_back(++event_count);
    };

    {
        auto& state = key_state_map[params.key];
        test_event_edge_values(state, state.value.x);

        // Accumulate deltas until next processing
        state.sample_count_accumulator += params.num_samples;
        state.raw_value_accumulator.x += params.amount_pressed;
    }

    // Mirror the key press to any associated paired axis
    const auto paired_key = details.get_paired_axis_key();
    if (paired_key != Key::Invalid)
    {
        auto& paired_details = KeyMapping::get_details(paired_key);

        const auto paired_axis = details.get_paired_axis();
        auto& paired_state = key_state_map[paired_key];

        // DenseMap can move values on add, therefore the old reference might be invalid, and we need a new lookup
        const auto& state = key_state_map[params.key];

        switch (paired_axis)
        {
        case PairedAxis::X:
            paired_state.raw_value_accumulator.x = state.raw_value_accumulator.x;
            paired_state.pair_sampled_axes |= 0b001;
            break;
        case PairedAxis::Y:
            paired_state.raw_value_accumulator.y = state.raw_value_accumulator.x;
            paired_state.pair_sampled_axes |= 0b010;
            break;
        case PairedAxis::Z:
            paired_state.raw_value_accumulator.z = state.raw_value_accumulator.x;
            paired_state.pair_sampled_axes |= 0b100;
            break;
        case PairedAxis::Unpaired:
            LOG_ERROR("Tried to mirror paired axis to unpaired key");
            break;
        }
        paired_state.sample_count_accumulator = std::max(paired_state.sample_count_accumulator, state.sample_count_accumulator);

        test_event_edge_values(paired_state, glm::length(paired_state.value));
    }

    return false;
}

bool Input::process_digital_event(const InputKeyEventParams& params)
{
    auto& details = KeyMapping::get_details(params.key);
    auto& state = key_state_map[params.key];

    const float elapsed_seconds = timer.elapsed<Timer::Seconds>();

    if (state.just_flushed && params.event == InputEventType::Repeat && state.event_accumulator[static_cast<uint8_t>(params.event)].empty())
    {
        state.raw_value_accumulator.x = params.amount_pressed;
        state.event_accumulator[static_cast<uint8_t>(params.event)].emplace_back(++event_count);
        state.last_state_change_transition_time = elapsed_seconds;
        state.sample_count_accumulator++;
    }

    switch (params.event)
    {
    case InputEventType::Repeat:
        [[fallthrough]]
    case InputEventType::Pressed:
        state.raw_value_accumulator.x = params.amount_pressed;
        state.event_accumulator[static_cast<uint8_t>(params.event)].emplace_back(++event_count);
        if (state.down_previous == false)
        {
            constexpr float double_click_time = 0.2f; // TODO: Get this from settings
            if (elapsed_seconds - state.last_state_change_transition_time < double_click_time)
            {
                state.event_accumulator[static_cast<uint8_t>(InputEventType::DoubleClick)].emplace_back(++event_count);
            }
            state.last_state_change_transition_time = elapsed_seconds;
        }
    case InputEventType::Released:
        state.raw_value_accumulator.x = 0.f;
        state.event_accumulator[static_cast<uint8_t>(InputEventType::Released)].emplace_back(++event_count);
        break;
    case InputEventType::DoubleClick:
        state.raw_value_accumulator.x = params.amount_pressed;
        state.event_accumulator[static_cast<uint8_t>(InputEventType::Pressed)].emplace_back(++event_count);
        state.event_accumulator[static_cast<uint8_t>(InputEventType::DoubleClick)].emplace_back(++event_count);
    default:
        break;
    }
    state.sample_count_accumulator++;

    state.just_flushed = false;

    if (params.event == InputEventType::Pressed)
    {
        return is_key_handled_by_action(params.key);
    }

    return true;
}

bool Input::is_key_handled_by_action(const Key& key) const
{
    for (const auto& mapping : action_mappings)
    {
        if ((mapping.key == key || mapping.key == Key::Any) &&
            (mapping.alt == false || is_alt_pressed()) &&
            (mapping.ctrl == false || is_ctrl_pressed()) &&
            (mapping.shift == false || is_shift_pressed()) &&
            (mapping.system == false || is_system_pressed())
        )
            return true;
    }

    return false;
}

void Input::evaluate_key_states(float delta_time, llvm::SmallVectorImpl<std::pair<Key, KeyState*>>& keys_with_events)
{
    for (auto& [key, state] : key_state_map)
    {
        auto& details = KeyMapping::get_details(key);

        bool has_events = false;
        for (auto event_index = 0; event_index < state.event_counts->size(); ++event_index)
        {
            state.event_counts[event_index].clear();
            std::swap(state.event_counts[event_index], state.event_accumulator[event_index]);

            if (!has_events && state.event_counts[event_index].size() > 0)
            {
                keys_with_events.emplace_back(key, &state);
                has_events = true;
            }
        }

        if (state.sample_count_accumulator > 0 || details.should_update_axis_without_samples())
        {
            if (state.pair_sampled_axes)
            {
                for (int32_t axis = 0; axis < 3; ++axis)
                {
                    if (state.pair_sampled_axes & (1 << axis))
                    {
                        state.raw_value[axis] = state.raw_value_accumulator[axis];
                    }
                }
            }
            else
            {
                state.raw_value = state.raw_value_accumulator;
            }

            // If we have no samples we assume the state hasn't changed
            if (state.sample_count_accumulator == 0)
            {
                state.event_counts[InputEventType::Released].emplace_back(++event_count);
                if (!has_events)
                {
                    keys_with_events.emplace_back(key, &state);
                    has_events = true;
                }
            }
        }


        process_non_axes_keys(key, state);

        state.raw_value_accumulator = glm::vec3(0);
        state.sample_count_accumulator = 0;
        state.pair_sampled_axes = 0;
    }
    event_count = 0;
}

void Input::process_non_axes_keys(Key, KeyState& state)
{
    // auto& details = KeyMapping::get_details(key);

    // if (details.is_axis_2D() || details.is_axis_3D())
    // {
    //     state.value = massage_vector_axis_input(key, state.raw_value);
    // }
    // else
    // {
    //     state.value.x = massage_axis_input(key, state.raw_value.x);
    // }

    const auto press_delta = state.event_counts[static_cast<uint8_t>(InputEventType::Pressed)].size() - state.event_counts[static_cast<uint8_t>(
        InputEventType::Released)].size();
    if (press_delta < 0)
    {
        state.down = false;
    }
    else if (press_delta > 0)
    {
        state.down = true;
    }
    else
    {
        state.down = state.down_previous;
    }

}

}
