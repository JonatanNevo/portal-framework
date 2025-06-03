//
// Created by thejo on 5/23/2025.
//

#pragma once

#include "portal/application/window/render_target.h"
#include "portal/application/window/window.h"

namespace portal
{

struct ApplicationContext
{
    std::shared_ptr<Window> window = nullptr;
    std::shared_ptr<RenderTarget> render_target = nullptr;
};

}
