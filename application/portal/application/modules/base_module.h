//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/core/strings/string_id.h>

#include "portal/application/modules/tags.h"

namespace portal
{
struct FrameContext;

class Event;

class BaseModule
{
public:
    explicit BaseModule(const StringId& name) : name(name) {}
    virtual ~BaseModule() = default;

    virtual void begin_frame(FrameContext& frame);

    virtual void update(FrameContext& frame);
    virtual void gui_update(FrameContext& frame);

    virtual void post_update(FrameContext& frame);

    virtual void end_frame(FrameContext& frame);

    virtual void on_event(Event& event);

    [[nodiscard]] const StringId& get_name() const { return name; }
    [[nodiscard]] virtual std::vector<BaseModule*> get_dependencies() const { return {}; }

    template <ModuleTags T>
    bool has_tag()
    {
        return has_tag(Tag<T>::Tags);
    }

    template <ModuleTags... T>
    bool has_tags()
    {
        std::array query = {Tag<T>::Tags...};

        bool res = true;
        for (auto id : query)
        {
            res &= has_tag(id);
        }
        return res;
    }

    [[nodiscard]] virtual bool has_tag(TagFlag tag) const = 0;

protected:
    StringId name;
};
}
