//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <filesystem>

#include "portal/engine/reference.h"
#include "portal/core/events/event_handler.h"
#include "portal/core/strings/string_id.h"

namespace portal
{

class InputEventConsumer;
class WindowEventConsumer;

}

namespace portal
{
namespace renderer
{
    namespace vulkan
    {
        class VulkanContext;
    }

    class Surface;
}

struct WindowExtent
{
    size_t width;
    size_t height;
};

enum class WindowMode
{
    Headless,
    Fullscreen,
    FullscreenBorderless,
    Default,
};

struct WindowProperties
{
    StringId title = STRING_ID("Portal");
    // TODO: change it from here :(
    std::filesystem::path icon_path = R"(C:\Code\portal-framework\engine\resources\portal_icon_64x64.png)";
    WindowExtent extent{1280, 720};

    WindowMode mode = WindowMode::Default;
    bool resizeable = true;
    bool vsync = true;
    bool decorated = true;

    size_t requested_frames_in_flight = 3;
};

struct CallbackConsumers
{
    std::reference_wrapper<WindowEventConsumer> window;
    std::reference_wrapper<InputEventConsumer> input;
};

class Window : public EventHandler
{
public:
    /**
    * Constructs a Window
    * @param properties The preferred configuration of the window
    */
    Window(const WindowProperties& properties, const CallbackConsumers& consumers);

    /**
      * Handles the processing of all underlying window events
      */
    virtual void process_events() = 0;

    /**
     * Checks if the window should be closed
     */
    [[nodiscard]] virtual bool should_close() const = 0;

    /**
     * Request a close to the window.
     * NOTE: this will trigger a corresponding window event
     */
    virtual void close() = 0;

    /**
     * Attempts to resize the window to the requested extent - not guaranteed to change
     *
     * @param requested_extent The preferred window extent.
     * @return The new window extent, based on window limitation and mode
     */
    WindowExtent resize(const WindowExtent& requested_extent);

    /**
     * Creates a surface inside a gpu context and returns it
     * NOTE: The window is not the owner of the surface and is not responsible for cleaning it before destruction.
     *
     * @param context The GPU Context to create the surface in TODO: rename `VulkanContext` to `GpuContext`
     * @returns An owning reference to the Surface created
     */
    [[nodiscard]] virtual Reference<renderer::Surface> create_surface(const renderer::vulkan::VulkanContext& context) = 0;

    /**
     * @return The dot-per-inch scale factor
     */
    [[nodiscard]] virtual float get_dpi_factor() const = 0;

    /**
     * @return The scale factor for systems with heterogeneous window and pixel coordinates
     */
    [[nodiscard]] virtual float get_content_scale_factor() const;

    virtual void maximize() = 0;
    virtual void center_window() = 0;

    virtual void set_vsync(bool enable) = 0;
    virtual void set_resizeable(bool enable) = 0;
    virtual void set_title(StringId title) = 0;

    [[nodiscard]] virtual glm::vec2 get_position() const = 0;
    [[nodiscard]] size_t get_width() const { return properties.extent.width; };
    [[nodiscard]] size_t get_height() const { return properties.extent.height; };
    [[nodiscard]] WindowExtent get_extent() const { return properties.extent; }
    [[nodiscard]] StringId get_title() const { return properties.title; }
    [[nodiscard]] bool is_resizeable() const { return properties.resizeable; }
    [[nodiscard]] WindowMode get_mode() const { return properties.mode; }
    [[nodiscard]] bool is_vsynced() const { return properties.vsync; }

    [[nodiscard]] const WindowProperties& get_properties() const { return properties; }

protected:
    WindowProperties properties;
    CallbackConsumers consumers;
};

} // portal
