//
// Created by Jonatan Nevo on 31/01/2025.
//

#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "portal/application/application_context.h"
#include "portal/application/layer.h"
#include "portal/core/timer.h"

namespace portal
{
    struct ApplicationSettings
    {
        std::string name = "Portal Application";
        bool platform_timer = false;
        float main_loop_sleep_duration = 0.f;
        WindowSettings window_settings{};
    };


    class Application
    {
    public:
        explicit Application(const ApplicationSettings& specs = ApplicationSettings());
        virtual ~Application();

        virtual void run();
        virtual void close();

        template <typename T>
            requires std::is_base_of_v<Layer, T>
        void push_layer()
        {
            push_layer(std::move(std::make_shared<T>()));
        }

        void push_layer(const std::shared_ptr<Layer>& layer)
        {
            layer_stack.emplace_back(layer);
            layer->on_context_change(&context);
            layer->on_attach(this);
        }

        std::vector<std::shared_ptr<Layer>>& get_layer_stack() { return layer_stack; }
        std::function<float()> get_time;

    protected:
        void init();
        void shutdown();

    protected:
        std::vector<std::shared_ptr<Layer>> layer_stack;
        ApplicationContext context;
        ApplicationSettings settings;

        bool running = false;

        Timer timer;
        float time_step = 0.f;
        float frame_time = 0.f;
        float last_frame_time = 0.f;
    };

    Application* create_application(int argc, char** argv);

} // namespace portal
