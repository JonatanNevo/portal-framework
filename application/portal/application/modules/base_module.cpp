//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "base_module.h"

namespace portal
{
void BaseModule::begin_frame(renderer::FrameContext&) {}

void BaseModule::gui_update(renderer::FrameContext&) {}

void BaseModule::post_update(renderer::FrameContext&) {}

void BaseModule::end_frame(renderer::FrameContext&) {}

void BaseModule::on_event(Event&) {}

void BaseModule::update(renderer::FrameContext&) {}
}
