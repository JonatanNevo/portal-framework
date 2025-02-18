//
// Created by Jonatan Nevo on 31/01/2025.
//

#include "gui_application.h"

#include "imgui_internal.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <stb_image.h>
#include <vulkan/vulkan.hpp>

#include "portal/core/assert.h"
#include "portal/core/log.h"

#include "portal/gui/image.h"
#include "portal/gui/ui/utils.h"


extern bool g_application_running;

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// #define IMGUI_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif

static vk::Instance g_instance = nullptr;
static vk::PhysicalDevice g_physical_device = nullptr;
static vk::Device g_device = nullptr;
static uint32_t g_queue_family = -1;
static vk::Queue g_queue = nullptr;
static vk::DebugReportCallbackEXT g_debug_report = nullptr;
static vk::PipelineCache g_pipeline_cache = nullptr;
static vk::DescriptorPool g_descriptor_pool = nullptr;

static ImGui_ImplVulkanH_Window g_main_window_data;
static int g_min_image_count = 2;
static bool g_swap_chain_rebuild = false;

// per france in flight
static std::vector<std::vector<vk::CommandBuffer>> s_allocated_command_buffers;
static std::vector<std::vector<std::function<void()>>> s_resource_free_queue;

static vk::CommandBuffer s_active_command_buffer = nullptr;

// Unlike g_MainWindowData.FrameIndex, this is not the the swapchain image index
// and is always guaranteed to increase (eg. 0, 1, 2, 0, 1, 2)
static uint32_t s_current_frame_index = 0;

static std::unordered_map<std::string, ImFont> s_fonts;

static portal::GUIApplication* s_instance = nullptr;

void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
    {
        PORTAL_DEBUG_BREAK;
        abort();
    }
}

static void setup_vulkan(const char** extensions, uint32_t extensions_count)
{
    {
        vk::ApplicationInfo application_info("Portal", VK_MAKE_VERSION(1, 0, 0), "Portal Engine", VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_0);
        vk::InstanceCreateInfo create_info({}, &application_info);
        create_info.enabledExtensionCount = extensions_count;
        create_info.ppEnabledExtensionNames = extensions;

#ifdef IMGUI_VULKAN_DEBUG_REPORT
        const char* layers[] = {"VK_LAYER_KHRONOS_validation"};
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = layers;

        // Enable debug report extension (we need additional storage, so we duplicate the user array to add our new extension to it)
        const char** extensions_ext = (const char**)malloc(sizeof(const char*) * (extensions_count + 1));
        memcpy(extensions_ext, extensions, extensions_count * sizeof(const char*));
        extensions_ext[extensions_count] = "VK_EXT_debug_report";
        create_info.enabledExtensionCount = extensions_count + 1;
        create_info.ppEnabledExtensionNames = extensions_ext;

        g_instance = vk::createInstance(create_info);
        free(extensions_ext);
        IM_UNUSED(g_debug_report);
#else
        g_instance = vk::createInstance(create_info);
        IM_UNUSED(g_debug_report);
#endif
    }

    // Select gpu
    {
        auto devices = g_instance.enumeratePhysicalDevices();
        IM_ASSERT(devices.size() > 0);

        for (const auto& gpu : devices)
        {
            if (gpu.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                g_physical_device = gpu;
                break;
            }
        }
    }

    // Select graphics queue family
    {
        auto families = g_physical_device.getQueueFamilyProperties();
        for (int i = 0; i < families.size(); i++)
        {
            if (families[i].queueFlags & vk::QueueFlagBits::eGraphics)
            {
                g_queue_family = i;
                break;
            }
        }

        IM_ASSERT(g_queue_family != -1);
    }

    // Create logical device (with 1 queue)
    {
        const char* device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        constexpr float queue_priority[] = {1.0f};
        vk::DeviceQueueCreateInfo queue_info({}, g_queue_family, 1, queue_priority);

        const vk::DeviceCreateInfo info({}, queue_info, nullptr, device_extensions);
        g_device = g_physical_device.createDevice(info);
        g_queue = g_device.getQueue(g_queue_family, 0);
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
            {vk::DescriptorType::eInputAttachment, 1000}};

        const vk::DescriptorPoolCreateInfo info(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1000 * IM_ARRAYSIZE(pool_sizes), pool_sizes);
        g_descriptor_pool = g_device.createDescriptorPool(info);
    }
}

static void setup_vulkan_window(ImGui_ImplVulkanH_Window* window, vk::SurfaceKHR surface, int width, int height)
{
    window->Surface = surface;

    auto res = g_physical_device.getSurfaceSupportKHR(g_queue_family, window->Surface);
    if (res != VK_TRUE)
    {
        std::cerr << "Error: Queue family does not support presentation" << std::endl;
        return;
    }

    constexpr VkFormat request_surface_image_format[] = {
        VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};
    constexpr VkColorSpaceKHR request_surface_color_space = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    window->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
        g_physical_device, window->Surface, request_surface_image_format, IM_ARRAYSIZE(request_surface_image_format), request_surface_color_space);

    // Select Present Mode
#ifdef IMGUI_UNLIMITED_FRAME_RATE
    VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR};
#else
    VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};
#endif

    window->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(g_physical_device, window->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));

    // Create SwapChain, RenderPass, Framebuffer, etc.
    IM_ASSERT(g_min_image_count >= 2);
    ImGui_ImplVulkanH_CreateOrResizeWindow(
        g_instance, g_physical_device, g_device, window, g_queue_family, nullptr, width, height, g_min_image_count);
}

static void cleanup_vulkan()
{
    g_device.destroyDescriptorPool(g_descriptor_pool);
    g_device.destroy();
    g_instance.destroy();
}

static void cleanup_vulkan_window() { ImGui_ImplVulkanH_DestroyWindow(g_instance, g_device, &g_main_window_data, nullptr); }

static void frame_render(portal::GUIApplication* application, ImGui_ImplVulkanH_Window* window, ImDrawData* draw_data)
{
    VkSemaphore image_acquired_semaphore = window->FrameSemaphores[s_current_frame_index].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = window->FrameSemaphores[s_current_frame_index].RenderCompleteSemaphore;
    auto res = g_device.acquireNextImageKHR(window->Swapchain, UINT64_MAX, image_acquired_semaphore, nullptr, &window->FrameIndex);
    if (res == vk::Result::eErrorOutOfDateKHR || res == vk::Result::eSuboptimalKHR)
    {
        g_swap_chain_rebuild = true;
        return;
    }
    if (res != vk::Result::eSuccess)
    {
        std::cerr << "Error: Failed to acquire next image" << std::endl;
        return;
    }

    s_current_frame_index = (s_current_frame_index + 1) % window->ImageCount;
    ImGui_ImplVulkanH_Frame* fd = &window->Frames[window->FrameIndex];
    {
        // wait indefinitely instead of periodically checking
        auto err = vkWaitForFences(g_device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);
        if (err != VK_SUCCESS)
        {
            std::cerr << "Error: Failed to wait for fence" << std::endl;
            return;
        }

        err = vkResetFences(g_device, 1, &fd->Fence);
        if (err != VK_SUCCESS)
        {
            std::cerr << "Error: Failed to reset fence" << std::endl;
            return;
        }
    }

    {
        for (auto& callback : s_resource_free_queue[s_current_frame_index])
            callback();
        s_resource_free_queue[s_current_frame_index].clear();
    }

    {
        // Free command buffers allocated by Application::GetCommandBuffer
        // These use g_MainWindowData.FrameIndex and not s_CurrentFrameIndex because they're tied to the swapchain image index
        auto& allocated_command_buffers = s_allocated_command_buffers[window->FrameIndex];
        if (allocated_command_buffers.size() > 0)
        {
            g_device.freeCommandBuffers(fd->CommandPool, allocated_command_buffers);
            allocated_command_buffers.clear();
        }

        g_device.resetCommandPool(fd->CommandPool, {});

        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(fd->CommandBuffer, &info);

        s_active_command_buffer = fd->CommandBuffer;
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

    for (const auto& layer : application->get_layer_stack())
        layer->on_render();

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
        s_active_command_buffer = nullptr;

        vkQueueSubmit(g_queue, 1, &info, fd->Fence);
    }
}

static void frame_resent(ImGui_ImplVulkanH_Window* wd)
{
    if (g_swap_chain_rebuild)
        return;
    const VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->Swapchain;
    info.pImageIndices = &wd->FrameIndex;
    VkResult err = vkQueuePresentKHR(g_queue, &info);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
    {
        g_swap_chain_rebuild = true;
        return;
    }
    if (err != VK_SUCCESS)
    {
        std::cerr << "Error: Failed to present" << std::endl;
        return;
    }
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->ImageCount; // Now we can use the next set of semaphores
}

static void glfw_error_callback(int error, const char* description) { fprintf(stderr, "Glfw Error %d: %s\n", error, description); }

namespace portal
{
    GUIApplication::GUIApplication(const ApplicationSpecs& specs) : specs(specs)
    {
        s_instance = this;
        init();
    }

    GUIApplication::~GUIApplication()
    {
        shutdown();
        s_instance = nullptr;
    }

    GUIApplication& GUIApplication::get() { return *s_instance; }

    void GUIApplication::init()
    {
        // Intialize logging
        Log::init();

        // Setup GLFW window
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
        {
            std::cerr << "Could not initalize GLFW!\n";
            return;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(primary_monitor);

        int monitorx, monitory;
        glfwGetMonitorPos(primary_monitor, &monitorx, &monitory);

        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        window_handle = glfwCreateWindow(specs.width, specs.height, specs.name.c_str(), nullptr, nullptr);

        if (specs.center_window)
        {
            glfwSetWindowPos(window_handle, monitorx + (mode->width - specs.width) / 2, monitory + (mode->height - specs.height) / 2);
            glfwSetWindowAttrib(window_handle, GLFW_RESIZABLE, specs.resizeable ? GLFW_TRUE : GLFW_FALSE);
        }

        glfwShowWindow(window_handle);

        if (!glfwVulkanSupported())
        {
            LOG_CORE_ERROR_TAG("App", "Vulkan not supported!");
            return;
        }

        GLFWimage icon;
        int channels;
        if (!specs.icon_path.empty())
        {
            std::string icon_path_string = specs.icon_path.string();
            icon.pixels = stbi_load(icon_path_string.c_str(), &icon.width, &icon.height, &channels, STBI_rgb_alpha);
            glfwSetWindowIcon(window_handle, 1, &icon);
            stbi_image_free(icon.pixels);
        }

        glfwSetWindowUserPointer(window_handle, this);

        uint32_t extension_count = 0;
        const char** extensions = glfwGetRequiredInstanceExtensions(&extension_count);
        setup_vulkan(extensions, extension_count);

        // Create Window Surface
        VkSurfaceKHR surface;
        VkResult err = glfwCreateWindowSurface(g_instance, window_handle, nullptr, &surface);
        check_vk_result(err);

        int w, h;
        glfwGetFramebufferSize(window_handle, &w, &h);
        ImGui_ImplVulkanH_Window* window = &g_main_window_data;
        setup_vulkan_window(window, surface, w, h);

        s_allocated_command_buffers.resize(window->ImageCount);
        s_resource_free_queue.resize(window->ImageCount);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows

        // Set custom themes
        // UI::SetHazelTheme();



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
        ImGui_ImplGlfw_InitForVulkan(window_handle, true);

        ImGui_ImplVulkan_InitInfo init_info{};
        init_info.Instance = g_instance;
        init_info.PhysicalDevice = g_physical_device;
        init_info.Device = g_device;
        init_info.QueueFamily = g_queue_family;
        init_info.Queue = g_queue;
        init_info.PipelineCache = g_pipeline_cache;
        init_info.DescriptorPool = g_descriptor_pool;
        init_info.Subpass = 0;
        init_info.MinImageCount = g_min_image_count;
        init_info.ImageCount = window->ImageCount;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = check_vk_result;
        init_info.RenderPass = window->RenderPass;
        ImGui_ImplVulkan_Init(&init_info);

        // Load default font'
        // ImFontConfig fontConfig;
        // fontConfig.FontDataOwnedByAtlas = false;
        // ImFont* robotoFont = io.Fonts->AddFontFromMemoryTTF((void*)g_RobotoRegular, sizeof(g_RobotoRegular), 20.0f, &fontConfig);
        // s_Fonts["Default"] = robotoFont;
        // s_Fonts["Bold"] = io.Fonts->AddFontFromMemoryTTF((void*)g_RobotoBold, sizeof(g_RobotoBold), 20.0f, &fontConfig);
        // s_Fonts["Italic"] = io.Fonts->AddFontFromMemoryTTF((void*)g_RobotoItalic, sizeof(g_RobotoItalic), 20.0f, &fontConfig);
        // io.FontDefault = robotoFont;

        // // Upload Fonts
        // {
        //     // Use any command queue
        //     VkCommandPool command_pool = wd->Frames[wd->FrameIndex].CommandPool;
        //     VkCommandBuffer command_buffer = wd->Frames[wd->FrameIndex].CommandBuffer;
        //
        //     err = vkResetCommandPool(g_Device, command_pool, 0);
        //     check_vk_result(err);
        //     VkCommandBufferBeginInfo begin_info = {};
        //     begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        //     begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        //     err = vkBeginCommandBuffer(command_buffer, &begin_info);
        //     check_vk_result(err);
        //
        //     ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
        //
        //     VkSubmitInfo end_info = {};
        //     end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        //     end_info.commandBufferCount = 1;
        //     end_info.pCommandBuffers = &command_buffer;
        //     err = vkEndCommandBuffer(command_buffer);
        //     check_vk_result(err);
        //     err = vkQueueSubmit(g_Queue, 1, &end_info, VK_NULL_HANDLE);
        //     check_vk_result(err);
        //
        //     err = vkDeviceWaitIdle(g_Device);
        //     check_vk_result(err);
        //     ImGui_ImplVulkan_DestroyFontUploadObjects();
        // }

        // Load images
        // {
        //     uint32_t w, h;
        //     void* data = Image::Decode(g_WalnutIcon, sizeof(g_WalnutIcon), w, h);
        //     m_AppHeaderIcon = std::make_shared<Walnut::Image>(w, h, ImageFormat::RGBA, data);
        //     free(data);
        // }
        // {
        //     uint32_t w, h;
        //     void* data = Image::Decode(g_WindowMinimizeIcon, sizeof(g_WindowMinimizeIcon), w, h);
        //     m_IconMinimize = std::make_shared<Walnut::Image>(w, h, ImageFormat::RGBA, data);
        //     free(data);
        // }
        // {
        //     uint32_t w, h;
        //     void* data = Image::Decode(g_WindowMaximizeIcon, sizeof(g_WindowMaximizeIcon), w, h);
        //     m_IconMaximize = std::make_shared<Walnut::Image>(w, h, ImageFormat::RGBA, data);
        //     free(data);
        // }
        // {
        //     uint32_t w, h;
        //     void* data = Image::Decode(g_WindowRestoreIcon, sizeof(g_WindowRestoreIcon), w, h);
        //     m_IconRestore = std::make_shared<Walnut::Image>(w, h, ImageFormat::RGBA, data);
        //     free(data);
        // }
        // {
        //     uint32_t w, h;
        //     void* data = Image::Decode(g_WindowCloseIcon, sizeof(g_WindowCloseIcon), w, h);
        //     m_IconClose = std::make_shared<Walnut::Image>(w, h, ImageFormat::RGBA, data);
        //     free(data);
        // }
    }


    void GUIApplication::shutdown()
    {
        for (auto& layer : layer_stack)
            layer->on_detach();

        layer_stack.clear();

        app_header_icon.reset();
        icon_close.reset();
        icon_minimize.reset();
        icon_maximize.reset();
        icon_restore.reset();

        g_device.waitIdle();

        for (auto& queue : s_resource_free_queue)
        {
            for (auto& callback : queue)
                callback();
        }
        s_resource_free_queue.clear();

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        cleanup_vulkan_window();
        cleanup_vulkan();

        glfwDestroyWindow(window_handle);
        glfwTerminate();

        g_application_running = false;

        Log::shutdown();
    }

    void GUIApplication::ui_draw_title_bar(float& out_height)
    {
        const float title_bar_height = 58.0f;
        const bool maximised = is_maximized();
        float title_bar_vertical_offset = maximised ? -6.f : 0.f;
        const ImVec2 window_padding = ImGui::GetCurrentWindow()->WindowPadding;

        ImGui::SetCursorPos(ImVec2(window_padding.x, window_padding.y + title_bar_vertical_offset));
        const ImVec2 title_bar_min = ImGui::GetCursorScreenPos();
        const ImVec2 title_bar_max = {ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth() - window_padding.y * 2.0f,
        ImGui::GetCursorScreenPos().y + title_bar_height};

        auto* bg_draw_list = ImGui::GetBackgroundDrawList();
        auto* fg_draw_list = ImGui::GetForegroundDrawList();
        bg_draw_list->AddRectFilled(title_bar_min, title_bar_max, IM_COL32(0, 0, 0, 255)); //UI::Colors::Theme::titlebar

        {
            const int logo_width = 48;
            const int logo_height = 48;
            const ImVec2 logo_offset(16.f + window_padding.x, 5.f + window_padding.y + title_bar_vertical_offset);
            const ImVec2 logo_rect_start = {ImGui::GetItemRectMin().x + logo_offset.x, ImGui::GetItemRectMin().y + logo_offset.y};
            const ImVec2 logo_rect_max = {logo_rect_start.x + logo_width, logo_rect_start.y + logo_height};
            fg_draw_list->AddImage(reinterpret_cast<ImU64>(app_header_icon->get_descriptor_set()), logo_rect_start, logo_rect_max);
        }

        out_height = title_bar_height;
    }

    void GUIApplication::ui_draw_menu_bar()
    {
        if (!menu_bar_callback)
            return;

        if (ImGui::BeginMenuBar())
        {
            menu_bar_callback();
            ImGui::EndMenuBar();
        }
    }

    void GUIApplication::run()
    {
        running = true;

        ImGui_ImplVulkanH_Window* window = &g_main_window_data;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        ImGuiIO& io = ImGui::GetIO();

        while (!glfwWindowShouldClose(window_handle) && running)
        {
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            glfwPollEvents();

            {
                std::scoped_lock lock(event_queue_mutex);
                while (!event_queue.empty())
                {
                    auto callback = event_queue.front();
                    callback();
                    event_queue.pop();
                }
            }

            for (auto& layer : layer_stack)
                layer->on_update(time_step);

            // Resize swap chain?
            if (g_swap_chain_rebuild)
            {
                int width, height;
                glfwGetFramebufferSize(window_handle, &width, &height);
                if (width > 0 && height > 0)
                {
                    ImGui_ImplVulkan_SetMinImageCount(g_min_image_count);
                    ImGui_ImplVulkanH_CreateOrResizeWindow(g_instance, g_physical_device, g_device, &g_main_window_data, g_queue_family, nullptr, width, height, g_min_image_count);
                    g_main_window_data.FrameIndex = 0;

                    // Clear allocated command buffers from here since entire pool is destroyed
                    s_allocated_command_buffers.clear();
                    s_allocated_command_buffers.resize(g_main_window_data.ImageCount);

                    g_swap_chain_rebuild = false;
                }
            }

            // Start the Dear ImGui frame
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            if (specs.use_dock_space)
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
                if (!specs.custom_titlebar && menu_bar_callback)
                    window_flags |= ImGuiWindowFlags_MenuBar;

                const bool maximized = is_maximized();
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, maximized ? ImVec2(6.0f, 6.0f) : ImVec2(1.0f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f);

                ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
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

                if (specs.custom_titlebar)
                {
                    float titleBarHeight;
                    ui_draw_title_bar(titleBarHeight);
                    ImGui::SetCursorPosY(titleBarHeight);
                }

                // Dockspace
                ImGuiStyle& style = ImGui::GetStyle();
                float minWinSizeX = style.WindowMinSize.x;
                style.WindowMinSize.x = 370.0f;
                ImGui::DockSpace(ImGui::GetID("MyDockspace"));
                style.WindowMinSize.x = minWinSizeX;

                if (!specs.custom_titlebar)
                    ui_draw_menu_bar();

                for (auto& layer : layer_stack)
                    layer->on_ui_render();

                ImGui::End();
            }
            else
            {
                // No dockspace - just render windows
                for (auto& layer : layer_stack)
                    layer->on_ui_render();
            }

            // Rendering
            ImGui::Render();
            ImDrawData* main_draw_data = ImGui::GetDrawData();
            const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);
            window->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
            window->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
            window->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
            window->ClearValue.color.float32[3] = clear_color.w;
            if (!main_is_minimized)
                frame_render(this, window, main_draw_data);

            // Update and Render additional Platform Windows
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            // Present Main Platform Window
            if (!main_is_minimized)
                frame_resent(window);
            else
                std::this_thread::sleep_for(std::chrono::milliseconds(5));

            float time = get_time();
            frame_time = time - last_frame_time;
            time_step = glm::min<float>(frame_time, 0.0333f);
            last_frame_time = time;
        }
    }


    void GUIApplication::set_menubar_callback(const std::function<void()>& callback)
    {
        if (!specs.use_dock_space)
        {
            LOG_CORE_WARN_TAG("App", "Application::set_menubar_callback - ApplicationSpecification::use_dock_space is false so menubar will not be visible.");
        }
        menu_bar_callback = callback;
    }


    void GUIApplication::close()
    {
        running = false;
    }

    bool GUIApplication::is_maximized() const
    {
        return glfwGetWindowAttrib(window_handle, GLFW_MAXIMIZED);
    }

    float GUIApplication::get_time()
    {
        return static_cast<float>(glfwGetTime());
    }

    vk::Instance GUIApplication::get_instance() {
    return g_instance;
    }

    vk::PhysicalDevice GUIApplication::get_physical_device()
    {
         return g_physical_device;
    }

    vk::Device GUIApplication::get_device()
    {
        return g_device;
    }

    vk::CommandBuffer GUIApplication::get_command_buffer()
    {
        const ImGui_ImplVulkanH_Window* window = &g_main_window_data;
        const vk::CommandPool command_pool = window->Frames[window->FrameIndex].CommandPool;

        const vk::CommandBufferAllocateInfo info(command_pool, vk::CommandBufferLevel::ePrimary, 1);
        vk::CommandBuffer buffer = s_allocated_command_buffers[window->FrameIndex].emplace_back();
        buffer = g_device.allocateCommandBuffers(info)[0];

        const vk::CommandBufferBeginInfo begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        buffer.begin(begin_info);

        return buffer;
    }

    void GUIApplication::flush_command_buffer(vk::CommandBuffer command_buffer)
    { constexpr uint64_t DEFAULT_FENCE_TIMEOUT = 100000000000;
        vk::SubmitInfo end_info{{}, {}, command_buffer};
        command_buffer.end();

        vk::Fence fence = g_device.createFence({});
        g_queue.submit(end_info, fence);

        auto result = g_device.waitForFences(fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
        if (result != vk::Result::eSuccess)
            std::cerr << "Error: Failed to wait for fence" << std::endl;

        g_device.destroyFence(fence);
    }

    void GUIApplication::submit_resource_free(std::function<void()>&& func)
    {
        s_resource_free_queue[s_current_frame_index].emplace_back(std::move(func));
    }

    ImFont* GUIApplication::get_font(const std::string& string)
    {
        if (!s_fonts.contains(string))
            return nullptr;

        return &s_fonts[string];
    }

    ImGui_ImplVulkanH_Window* GUIApplication::get_main_window_data()
    {
        return &g_main_window_data;
    }

    vk::CommandBuffer GUIApplication::get_active_command_buffer()
    {
        return s_active_command_buffer;
    }

} // namespace portal
