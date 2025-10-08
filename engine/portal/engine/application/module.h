//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/strings/string_id.h"

namespace portal
{

class Module
{
public:
    explicit Module(const StringId& name): name(name) {}
    virtual ~Module() = default;

    virtual void on_attach() = 0;
    virtual void on_detach() = 0;

    virtual void on_update() = 0;
    virtual void on_gui_update() = 0;
    virtual void on_event() = 0;

    const StringId& get_name() const { return name; }

protected:
    StringId name;
};

}
