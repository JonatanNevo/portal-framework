//
// Created by thejo on 5/23/2025.
//

#pragma once

namespace portal
{
struct WindowSettings
{
    int width = 1920;
    int height = 1080;
    std::string title = "Portal";

    bool resizeable = true;
    bool center_window = false;
};

class Window
{
public:
    virtual ~Window() = default;

    virtual void initialize(WindowSettings settings) = 0;
    virtual void shutdown() = 0;

    virtual void poll_events() = 0;
    virtual float get_time() const = 0;

    virtual bool should_close() const = 0;
    virtual bool is_maximized() const = 0;
};
}
