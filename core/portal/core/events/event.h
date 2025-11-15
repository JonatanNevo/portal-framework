//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "event_types.h"
#include "portal/core/strings/string_id.h"

#define EVENT_CLASS_TYPE(type) constexpr static portal::EventType get_static_type() { return portal::EventType::type; }\
constexpr portal::EventType get_event_type() const override { return portal::EventType::type; }\
StringId get_name() const override { return STRING_ID(#type); }

#define EVENT_CLASS_CATEGORY(category) portal::EventCategory get_category() const override { using enum portal::EventCategoryBits; return category; }

namespace portal
{

class EventRunner;

class Event
{
public:
    virtual ~Event() = default;

    virtual EventType get_event_type() const = 0;
    virtual StringId get_name() const = 0;
    virtual EventCategory get_category() const = 0;
    virtual std::string to_string() const { return std::string(get_name().string); }

    [[nodiscard]] bool is_handled() const { return handled; }

protected:
    friend class EventRunner;

    bool handled = false;
};

// Type erasure for event running
// TODO: use variant instead of inheritance for event running?
class EventRunner
{
    template <typename T>
    using EventFunction = std::function<bool(T&)>;

public:
    explicit EventRunner(Event& event) : event(event) {}

    template <typename T>
    bool run_on(EventFunction<T> function)
    {
        if (event.get_event_type() == T::get_static_type() && !event.is_handled())
        {
            event.handled = function(*dynamic_cast<T*>(&event));
            return true;
        }
        return false;
    }

private:
    Event& event;
};
}
