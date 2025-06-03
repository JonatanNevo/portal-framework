//
// Created by thejo on 5/23/2025.
//

#pragma once
#include <string>
#include <vector>
#include <portal/core/glm.h>
#include <vulkan/vulkan.hpp>

namespace portal
{
class RenderTarget
{
public:
    virtual ~RenderTarget() = default;

    virtual std::vector<const char*> get_required_vulkan_extensions() = 0;

    virtual vk::SurfaceKHR create_surface(vk::Instance instance) = 0;

    virtual glm::ivec2 get_framebuffer_size() = 0;
};

}
