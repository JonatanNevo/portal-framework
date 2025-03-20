//
// Created by Jonatan Nevo on 01/03/2025.
//

#pragma once

#include "portal/application/window/window.h"

namespace portal
{
class HeadlessWindow final : public Window
{
public:
    HeadlessWindow(const Properties& properties);

    vk::SurfaceKHR create_surface(vulkan::Instance* instance) override;

    vk::SurfaceKHR create_surface(vk::Instance instance, vk::PhysicalDevice physical_device) override;

    bool should_close() override;

    void close() override;

    float get_dpi_factor() const override;

    std::vector<const char*> get_required_surface_extensions() const override;

private:
    bool closed{false};
};
} // portal
