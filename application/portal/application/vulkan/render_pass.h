//
// Created by Jonatan Nevo on 03/03/2025.
//

#pragma once
#include <string>
#include <vector>
#include "base/vulkan_resource.h"

namespace portal::vulkan
{
struct LoadStoreInfo;
struct Attachment;
class Device;

struct SubpassInfo
{
    std::vector<uint32_t> input_attachments;
    std::vector<uint32_t> output_attachments;
    std::vector<uint32_t> color_resolve_attachments;
    bool disable_depth_stencil_attachment;
    uint32_t depth_stencil_resolve_attachment;
    vk::ResolveModeFlagBits depth_stencil_resolve_mode;
    std::string debug_name;
};

class RenderPass final : public VulkanResource<vk::RenderPass>
{
public:
    RenderPass(
        Device& device,
        const std::vector<Attachment>& attachments,
        const std::vector<LoadStoreInfo>& load_store_infos,
        const std::vector<SubpassInfo>& subpasses
    );
    RenderPass(RenderPass&& other) noexcept;
    ~RenderPass() override;

    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;
    RenderPass& operator=(RenderPass&&) = delete;

    [[nodiscard]] uint32_t get_color_output_count(uint32_t subpass_index) const;
    [[nodiscard]] vk::Extent2D get_render_area_granularity() const;

private:
    size_t subpass_count;

    template <typename T_SubpassDescription, typename T_AttachmentDescription, typename T_AttachmentReference, typename T_SubpassDependency, typename T_RenderPassCreateInfo>
    void create_renderpass(const std::vector<Attachment> &attachments, const std::vector<LoadStoreInfo> &load_store_infos, const std::vector<SubpassInfo> &subpasses);
    std::vector<uint32_t> color_output_count;
};
} // portal
