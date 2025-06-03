//
// Created by thejo on 5/26/2025.
//

#pragma once
#include <memory>

namespace portal
{
/**
 * An interface for rendering UI elements with IMGUI, each call of `on_ui_render` is inside a ImGui frame.
 */
class UIRenderable: public std::enable_shared_from_this<UIRenderable>
{
public:
    virtual ~UIRenderable() = default;
    virtual void on_ui_render() = 0;
};
}
