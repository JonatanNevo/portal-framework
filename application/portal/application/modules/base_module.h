//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/core/strings/string_id.h>

#include "portal/application/modules/tags.h"

namespace portal
{
class Event;

namespace renderer
{
    struct FrameContext;
}

namespace tags
{
    struct PostUpdate {};
    struct FrameLifecycle {};
    struct Update {};
    struct Event {};
    struct Gui {};
}

class BaseModule
{
public:
    explicit BaseModule(const StringId& name) : name(name) {}
    virtual ~BaseModule() = default;

    // TODO: add module capabilities (Frame, Gui, Event)
    virtual void begin_frame(renderer::FrameContext& frame);

    virtual void update(renderer::FrameContext& frame);
    virtual void gui_update(renderer::FrameContext& frame);

    virtual void post_update(renderer::FrameContext& frame);

    virtual void end_frame(renderer::FrameContext& frame);

    virtual void on_event(Event& event);

    [[nodiscard]] const StringId& get_name() const { return name; }
    [[nodiscard]] virtual std::vector<BaseModule*> get_dependencies() const { return {}; }

    template <typename T>
    bool has_tag()
    {
        return has_tag(Tag<T>::ID);
    }

    template <typename... T>
    bool has_tags()
    {
        std::array query = {Tag<T>::ID...};

        bool res = true;
        for (auto id : query)
        {
            res &= has_tag(id);
        }
        return res;
    }

    [[nodiscard]] virtual bool has_tag(TagID id) const = 0;

protected:
    StringId name;
};
}
