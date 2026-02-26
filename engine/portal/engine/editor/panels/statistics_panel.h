//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "panel.h"

namespace portal
{

class StatisticsPanel final: public Panel
{
public:
    void on_gui_render(EditorContext& editor_context, FrameContext& frame_context, bool& is_open) override;
};
} // portal