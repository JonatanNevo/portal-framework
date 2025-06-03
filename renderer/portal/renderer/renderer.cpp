//
// Created by thejo on 5/23/2025.
//

#include "renderer.h"

#include <imgui_impl_glfw.h>
#include <imgui_internal.h>

#include "portal/application/application.h"
#include "portal/application/window/glfw_window.h"
#include "portal/core/assert.h"
#include "portal/renderer/ui/ui_renderable.h"
#include "portal/renderer/ui/utils.h"

#ifdef PORTAL_DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif


void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    LOG_CORE_ERROR_TAG("Vulkan", "Error: VkResult = {}", static_cast<int>(err));
    if (err < 0)
    {
        PORTAL_DEBUG_BREAK();
        abort();
    }
}


namespace portal
{
void Renderer::on_attach(Application* app)
{
    Layer::on_attach(app);

    const auto extensions = context->render_target->get_required_vulkan_extensions();
    setup_vulkan(extensions);

    auto surface = context->render_target->create_surface(instance);
    auto surface_extent = context->render_target->get_framebuffer_size();
    setup_vulkan_window(&window_data, surface, surface_extent.x, surface_extent.y);

    allocated_command_buffers.resize(window_data.ImageCount);
    resource_free_queue.resize(window_data.ImageCount);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

    // TODO: this is bad
    GLFWWindow* window = dynamic_cast<GLFWWindow*>(context->window.get());
    ImGui_ImplGlfw_InitForVulkan(window->get_handle(), true);

    // Style
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(8.0f, 6.0f);
    style.ItemSpacing = ImVec2(6.0f, 6.0f);
    style.ChildRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.FrameRounding = 6.0f;
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance = instance;
    init_info.PhysicalDevice = physical_device;
    init_info.Device = device;
    init_info.QueueFamily = queue_family;
    init_info.Queue = queue;
    init_info.PipelineCache = pipeline_cache;
    init_info.DescriptorPool = descriptor_pool;
    init_info.Subpass = 0;
    init_info.MinImageCount = min_image_count;
    init_info.ImageCount = window_data.ImageCount;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = check_vk_result;
    init_info.RenderPass = window_data.RenderPass;
    ImGui_ImplVulkan_Init(&init_info);
}

void Renderer::on_detach()
{
    Layer::on_detach();

    device.waitIdle();

    for (auto& queue : resource_free_queue)
    {
        for (auto& callback : queue)
            callback();
    }
    resource_free_queue.clear();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    cleanup_vulkan_window();
    cleanup_vulkan();
}

void Renderer::update(float dt)
{
    Layer::update(dt);

    constexpr auto clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGuiIO& io = ImGui::GetIO();

    // Resize swap chain?
    if (swap_chain_rebuild)
    {
        auto extent = context->render_target->get_framebuffer_size();
        if (extent.x > 0 && extent.y > 0)
        {
            ImGui_ImplVulkan_SetMinImageCount(min_image_count);
            ImGui_ImplVulkanH_CreateOrResizeWindow(
                instance,
                physical_device,
                device,
                &window_data,
                queue_family,
                nullptr,
                extent.x,
                extent.y,
                min_image_count
            );
            window_data.FrameIndex = 0;

            // Clear allocated command buffers from here since entire pool is destroyed
            allocated_command_buffers.clear();
            allocated_command_buffers.resize(window_data.ImageCount);

            swap_chain_rebuild = false;
        }
    }

    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // TODO: add renderer settings
    // if dockspace
    if (true)
    {
        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        const bool maximized = context->window->is_maximized();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, maximized ? ImVec2(6.0f, 6.0f) : ImVec2(1.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f);

        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4{0.0f, 0.0f, 0.0f, 0.0f});
        ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
        ImGui::PopStyleColor(); // MenuBarBg
        ImGui::PopStyleVar(2);

        ImGui::PopStyleVar(2);

        {
            ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(50, 50, 50, 255));
            // Draw window border if the window is not maximized
            if (!maximized)
                ui::render_window_outer_bounds(ImGui::GetCurrentWindow());

            ImGui::PopStyleColor(); // ImGuiCol_Border
        }

        // Dockspace
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 370.0f;
        ImGui::DockSpace(ImGui::GetID("MyDockspace"));
        style.WindowMinSize.x = minWinSizeX;

        for (auto& ui_renderable : ui_renderables)
            ui_renderable->on_ui_render();
    }
    else
    {
        // No dockspace - just render windows
        for (auto& ui_renderable : ui_renderables)
            ui_renderable->on_ui_render();
    }
    ImGui::End();

    // Rendering
    ImGui::Render();
    ImDrawData* main_draw_data = ImGui::GetDrawData();
    const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);
    window_data.ClearValue.color.float32[0] = clear_color.x * clear_color.w;
    window_data.ClearValue.color.float32[1] = clear_color.y * clear_color.w;
    window_data.ClearValue.color.float32[2] = clear_color.z * clear_color.w;
    window_data.ClearValue.color.float32[3] = clear_color.w;
    if (!main_is_minimized)
        frame_render(&window_data, main_draw_data);

    // Update and Render additional Platform Windows
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    // Present Main Platform Window
    if (!main_is_minimized)
        frame_present(&window_data);
}

void Renderer::add_ui_renderable(const std::shared_ptr<UIRenderable>& ui_renderable)
{
    ui_renderables.push_back(ui_renderable);
}

void Renderer::submit_resource_free(std::function<void()>&& func)
{
    if (!resource_free_queue.empty())
        resource_free_queue[current_frame_index].emplace_back(std::move(func));
}

vk::CommandBuffer Renderer::get_command_buffer()
{
    const ImGui_ImplVulkanH_Window* window = &window_data;
    const vk::CommandPool command_pool = window->Frames[window->FrameIndex].CommandPool;

    const vk::CommandBufferAllocateInfo info(command_pool, vk::CommandBufferLevel::ePrimary, 1);
    vk::CommandBuffer buffer = allocated_command_buffers[window->FrameIndex].emplace_back();
    buffer = device.allocateCommandBuffers(info)[0];

    const vk::CommandBufferBeginInfo begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    buffer.begin(begin_info);

    return buffer;
}

void Renderer::flush_command_buffer(vk::CommandBuffer command_buffer) const
{
    constexpr uint64_t DEFAULT_FENCE_TIMEOUT = 100000000000;
    vk::SubmitInfo end_info{{}, {}, command_buffer};
    command_buffer.end();

    vk::Fence fence = device.createFence({});
    queue.submit(end_info, fence);

    auto result = device.waitForFences(fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
    if (result != vk::Result::eSuccess)
        LOG_CORE_ERROR_TAG("Renderer", "Failed to wait for fence");

    device.destroyFence(fence);
}

void Renderer::setup_vulkan(std::vector<const char*> extensions)
{
    {
        vk::ApplicationInfo application_info("Portal", VK_MAKE_VERSION(1, 0, 0), "Portal Engine", VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_0);

        vk::InstanceCreateInfo create_info({}, &application_info);
        create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();

#ifdef IMGUI_VULKAN_DEBUG_REPORT
        const char* layers[] = {"VK_LAYER_KHRONOS_validation"};
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = layers;

        extensions.push_back("VK_EXT_debug_report");
        create_info.enabledExtensionCount = extensions.size() + 1;
        create_info.ppEnabledExtensionNames = extensions.data();

        instance = vk::createInstance(create_info);
        IM_UNUSED(debug_callback);
#else
        g_instance = vk::createInstance(create_info);
        IM_UNUSED(g_debug_report);
#endif
    }

    // Select gpu
    {
        auto devices = instance.enumeratePhysicalDevices();
        IM_ASSERT(devices.size() > 0);

        for (const auto& gpu : devices)
        {
            if (gpu.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                physical_device = gpu;
                break;
            }
        }
    }

    // Select graphics queue family
    {
        auto families = physical_device.getQueueFamilyProperties();
        for (int i = 0; i < families.size(); i++)
        {
            if (families[i].queueFlags & vk::QueueFlagBits::eGraphics)
            {
                queue_family = i;
                break;
            }
        }

        IM_ASSERT(queue_family != -1);
    }

    // Create logical device (with 1 queue)
    {
        const char* device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        constexpr float queue_priority[] = {1.0f};
        vk::DeviceQueueCreateInfo queue_info({}, queue_family, 1, queue_priority);

        const vk::DeviceCreateInfo info({}, queue_info, nullptr, device_extensions);
        device = physical_device.createDevice(info);
        queue = device.getQueue(queue_family, 0);
    }

    // Create descriptor Pool
    {
        vk::DescriptorPoolSize pool_sizes[] = {
            {vk::DescriptorType::eSampler, 1000},
            {vk::DescriptorType::eCombinedImageSampler, 1000},
            {vk::DescriptorType::eSampledImage, 1000},
            {vk::DescriptorType::eStorageImage, 1000},
            {vk::DescriptorType::eUniformTexelBuffer, 1000},
            {vk::DescriptorType::eStorageTexelBuffer, 1000},
            {vk::DescriptorType::eUniformBuffer, 1000},
            {vk::DescriptorType::eStorageBuffer, 1000},
            {vk::DescriptorType::eUniformBufferDynamic, 1000},
            {vk::DescriptorType::eStorageBufferDynamic, 1000},
            {vk::DescriptorType::eInputAttachment, 1000}
        };

        const vk::DescriptorPoolCreateInfo info(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1000 * IM_ARRAYSIZE(pool_sizes), pool_sizes);
        descriptor_pool = device.createDescriptorPool(info);
    }
}

void Renderer::setup_vulkan_window(ImGui_ImplVulkanH_Window* window, const vk::SurfaceKHR surface, const int width, const int height) const
{
    window->Surface = surface;

    const auto res = physical_device.getSurfaceSupportKHR(queue_family, window->Surface);
    if (res != VK_TRUE)
    {
        LOG_CORE_ERROR_TAG("Renderer", "Queue family does not support presentation");
        return;
    }

    constexpr VkFormat request_surface_image_format[] = {
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8_UNORM,
        VK_FORMAT_R8G8B8_UNORM
    };
    constexpr VkColorSpaceKHR request_surface_color_space = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    window->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
        physical_device,
        window->Surface,
        request_surface_image_format,
        IM_ARRAYSIZE(request_surface_image_format),
        request_surface_color_space
    );

    // Select Present Mode
#ifdef IMGUI_UNLIMITED_FRAME_RATE
    VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR};
#else
    VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};
#endif

    window->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(physical_device, window->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));

    // Create SwapChain, RenderPass, Framebuffer, etc.
    IM_ASSERT(min_image_count >= 2);
    ImGui_ImplVulkanH_CreateOrResizeWindow(
        instance,
        physical_device,
        device,
        window,
        queue_family,
        nullptr,
        width,
        height,
        min_image_count
    );
}

void Renderer::cleanup_vulkan() const
{
    device.destroyDescriptorPool(descriptor_pool);
    device.destroy();
    instance.destroy();
}

void Renderer::cleanup_vulkan_window()
{
    ImGui_ImplVulkanH_DestroyWindow(instance, device, &window_data, nullptr);
}

void Renderer::frame_render(ImGui_ImplVulkanH_Window* window, ImDrawData* draw_data)
{
    VkSemaphore image_acquired_semaphore = window->FrameSemaphores[current_frame_index].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = window->FrameSemaphores[current_frame_index].RenderCompleteSemaphore;
    auto res = device.acquireNextImageKHR(window->Swapchain, UINT64_MAX, image_acquired_semaphore, nullptr, &window->FrameIndex);
    if (res == vk::Result::eErrorOutOfDateKHR || res == vk::Result::eSuboptimalKHR)
    {
        swap_chain_rebuild = true;
        return;
    }
    if (res != vk::Result::eSuccess)
    {
        LOG_CORE_ERROR_TAG("Renderer", "Failed to acquire next image");
        return;
    }

    current_frame_index = (current_frame_index + 1) % window->ImageCount;
    ImGui_ImplVulkanH_Frame* fd = &window->Frames[window->FrameIndex];
    {
        // wait indefinitely instead of periodically checking
        auto err = vkWaitForFences(device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);
        if (err != VK_SUCCESS)
        {
            LOG_CORE_ERROR_TAG("Renderer", "Failed to wait for fence");
            return;
        }

        err = vkResetFences(device, 1, &fd->Fence);
        if (err != VK_SUCCESS)
        {
            LOG_CORE_ERROR_TAG("Renderer", "Failed to reset fence");
            return;
        }
    }

    {
        for (auto& callback : resource_free_queue[current_frame_index])
            callback();
        resource_free_queue[current_frame_index].clear();
    }

    {
        // Free command buffers allocated by Application::GetCommandBuffer
        // These use g_MainWindowData.FrameIndex and not s_CurrentFrameIndex because they're tied to the swapchain image index
        auto& command_buffers = allocated_command_buffers[window->FrameIndex];
        if (command_buffers.size() > 0)
        {
            device.freeCommandBuffers(fd->CommandPool, command_buffers);
            command_buffers.clear();
        }

        device.resetCommandPool(fd->CommandPool, {});

        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(fd->CommandBuffer, &info);

        active_command_buffer = fd->CommandBuffer;
    }

    {
        VkRenderPassBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = window->RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent = vk::Extent2D(window->Width, window->Height);
        info.clearValueCount = 1;
        info.pClearValues = &window->ClearValue;
        vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    // TODO: Call render for other renderable components?
    // for (const auto& layer : application->get_layer_stack())
    //     layer->on_render();

    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);
    vkCmdEndRenderPass(fd->CommandBuffer);

    // submit command buffer
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_acquired_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_complete_semaphore;
        vkEndCommandBuffer(fd->CommandBuffer);
        active_command_buffer = nullptr;

        vkQueueSubmit(queue, 1, &info, fd->Fence);
    }
}

void Renderer::frame_present(ImGui_ImplVulkanH_Window* window)
{
    if (swap_chain_rebuild)
        return;

    const VkSemaphore render_complete_semaphore = window->FrameSemaphores[window->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &window->Swapchain;
    info.pImageIndices = &window->FrameIndex;
    VkResult err = vkQueuePresentKHR(queue, &info);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
    {
        swap_chain_rebuild = true;
        return;
    }
    if (err != VK_SUCCESS)
    {
        LOG_CORE_ERROR_TAG("Renderer", "Failed to present");
        return;
    }
    window->SemaphoreIndex = (window->SemaphoreIndex + 1) % window->ImageCount; // Now we can use the next set of semaphores
}
} // portal
