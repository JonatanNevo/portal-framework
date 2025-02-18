//
// Created by Jonatan Nevo on 31/01/2025.
//

#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "portal/application/layer.h"

namespace portal
{
    struct ApplicationSpecs
    {
        std::string name = "Portal Application";
        size_t width = 1920;
        size_t height = 1080;
        uint64_t sleep_duration = 0;
        std::filesystem::path icon_path;

        bool resizeable = true;
        bool custom_titlebar = false;
        bool use_dock_space = true;
        bool center_window = false;
    };


    class Application
    {
    public:
        virtual ~Application() = default;

        virtual void run() = 0;

        template <typename T>
            requires std::is_base_of_v<Layer, T>
        void push_layer()
        {
            layer_stack.emplace_back(std::make_shared<T>())->on_attach();
        }

        void push_layer(const std::shared_ptr<Layer>& layer)
        {
            layer_stack.emplace_back(layer);
            layer->on_attach();
        }

        std::vector<std::shared_ptr<Layer>>& get_layer_stack() { return layer_stack; }

        virtual void close() = 0;
        virtual float get_time() = 0;

    protected:
        std::vector<std::shared_ptr<Layer>> layer_stack;
    };

    Application* create_application(int argc, char** argv);

} // namespace portal
