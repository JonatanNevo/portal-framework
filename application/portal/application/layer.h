//
// Created by Jonatan Nevo on 31/01/2025.
//

#pragma once

namespace portal
{

    class Layer
    {
    public:
        virtual ~Layer() = default;

        virtual void on_attach() {}
        virtual void on_detach() {}

        virtual void on_update(float dt) {}
        virtual void on_render() {}
        virtual void on_ui_render() {}
    };

} // namespace portal
