//
// Created by Jonatan Nevo on 31/01/2025.
//

#pragma once
#include <functional>
#include <mutex>
#include <queue>
#include <vulkan/vulkan.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "portal/application/application.h"

#define PORTAL_GUI

namespace portal
{
    class Image;

    class GUIApplication final : public Application
    {
    public:
        GUIApplication(const ApplicationSpecs& specs = ApplicationSpecs());
        ~GUIApplication();

        static GUIApplication& get();

        void run() override;
        void set_menubar_callback(const std::function<void()>& callback);

        void close() override;

        bool is_maximized() const;
        std::shared_ptr<Image> get_application_icon() const { return app_header_icon; }

        float get_time() override;
        static GLFWwindow* get_window_handle();
        bool is_title_bar_hovered() const { return title_bar_hovered; }

        static vk::Instance get_instance();
        static vk::PhysicalDevice get_physical_device();
        static vk::Device get_device();

        static vk::CommandBuffer get_command_buffer();
        static void flush_command_buffer(vk::CommandBuffer command_buffer);

        static void submit_resource_free(std::function<void()>&& func);

        static ImFont* get_font(const std::string& string);

        template <typename T>
        void queue_event(T&& func)
        {
            event_queue.push(func);
        }

        static ImGui_ImplVulkanH_Window* get_main_window_data();
        static vk::CommandBuffer get_active_command_buffer();

    private:
        void init();
        void shutdown();

        void ui_draw_title_bar(float& out_height);
        void ui_draw_menu_bar();
    private:
        ApplicationSpecs specs;
        GLFWwindow* window_handle = nullptr;
        bool running = false;

        float time_step = 0.0f;
        float last_frame_time = 0.0f;
        float frame_time = 0.0f;

        bool title_bar_hovered = false;

        std::function<void()> menu_bar_callback;

        std::mutex event_queue_mutex;
        std::queue<std::function<void()>> event_queue;

        std::shared_ptr<Image> app_header_icon;
        std::shared_ptr<Image> icon_close;
        std::shared_ptr<Image> icon_maximize;
        std::shared_ptr<Image> icon_minimize;
        std::shared_ptr<Image> icon_restore;
    };

} // namespace portal
