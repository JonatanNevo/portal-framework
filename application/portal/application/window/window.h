//
// Created by Jonatan Nevo on 01/03/2025.
//

#pragma once

#include <optional>
#include <string>

// TODO: uncouple window and vulkan
#include "portal/application/vulkan/common.h"

namespace portal
{
namespace vulkan {
    class Instance;
}

class Window
{
public:
    struct Extent
    {
        uint32_t width;
        uint32_t height;
    };

    struct OptionalExtent
    {
        std::optional<uint32_t> width;
        std::optional<uint32_t> height;
    };

    enum class Mode
    {
        Headless,
        Fullscreen,
        FullscreenBorderless,
        FullscreenStretch,
        Default
    };

    enum class Vsync
    {
        OFF,
        ON,
        Default
    };

    struct OptionalProperties
    {
        std::optional<std::string> title;
        std::optional<Mode> mode;
        std::optional<bool> resizable;
        std::optional<Vsync> vsync;
        OptionalExtent extent;
    };

    struct Properties
    {
        std::string title;
        Mode mode = Mode::Default;
        bool resizable = true;
        Vsync vsync = Vsync::Default;
        Extent extent = {1280, 720};
    };

public:
    /**
    * @brief Constructs a Window
    * @param properties The preferred configuration of the window
    */
    explicit Window(Properties properties);

    virtual ~Window() = default;

    /**
     * @brief Gets a handle from the platform's Vulkan surface
     * @param instance A Vulkan instance
     * @returns A vk::SurfaceKHR handle, for use by the application
     */
    virtual vk::SurfaceKHR create_surface(vulkan::Instance* instance) = 0;

    /**
     * @brief Gets a handle from the platform's Vulkan surface
     * @param instance A Vulkan instance
     * @param physical_device A Vulkan PhysicalDevice
     * @returns A vk::SurfaceKHR handle, for use by the application
    */
    virtual vk::SurfaceKHR create_surface(vk::Instance instance, vk::PhysicalDevice physical_device) = 0;

    /**
     * @brief Checks if the window should be closed
     */
    virtual bool should_close() = 0;

    /**
     * @brief Handles the processing of all underlying window events
     */
    virtual void process_events();

    /**
     * @brief Requests to close the window
     */
    virtual void close() = 0;

    /**
     * @return The dot-per-inch scale factor
     */
    [[nodiscard]] virtual float get_dpi_factor() const = 0;

    /**
     * @return The scale factor for systems with heterogeneous window and pixel coordinates
     */
    [[nodiscard]] virtual float get_content_scale_factor() const;

    /**
     * @brief Attempt to resize the window - not guaranteed to change
     *
     * @param extent The preferred window extent
     * @return Extent The new window extent
     */
    Extent resize(const Extent& extent);

    /**
     * @brief Get the display present info for the window if needed
     *
     * @param info Filled in when the method returns true
     * @param src_width The width of the surface being presented
     * @param src_height The height of the surface being presented
     * @return true if the present info was filled in and should be used
     * @return false if the extra present info should not be used. info is left untouched.
     */
    virtual bool get_display_present_info(vk::DisplayPresentInfoKHR* info, uint32_t src_width, uint32_t src_height) const;

    [[nodiscard]] virtual std::vector<const char*> get_required_surface_extensions() const = 0;

    [[nodiscard]] const Extent& get_extent() const;

    [[nodiscard]] Mode get_window_mode() const;

    [[nodiscard]] const Properties& get_properties() const
    {
        return properties;
    }

protected:
    Properties properties;
};
} // portal
