//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/events/event_types.h"
#include <portal/engine/strings/string_id.h>

namespace portal
{
#define EVENT_CLASS_TYPE(type) constexpr static portal::EventType get_static_type() { return portal::EventType::type; }\
constexpr portal::EventType get_event_type() const override { return portal::EventType::type; }\
StringId get_name() const override { return STRING_ID(#type); }

#define EVENT_CLASS_CATEGORY(category) portal::EventCategory get_category() const override { return portal::EventCategory::category; }

class Event
{
public:
    virtual ~Event() = default;

    virtual EventType get_event_type() const = 0;
    virtual StringId get_name() const = 0;
    virtual EventCategory get_category() const = 0;
    virtual std::string to_string() const { return std::string(get_name().string); }

    bool handled = false;
};

// Type erasure for event running
class EventDispatcher
{
    template<typename T>
    using EventFunction = std::function<bool(T&)>;

public:
    EventDispatcher(Event& event): event(event) {}

    template<typename T>
    bool dispatch(EventFunction<T> function)
    {
        if (event.get_event_type() == T::get_static_type() && !event.handled)
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
