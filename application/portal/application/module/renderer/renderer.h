//
// Created by Jonatan Nevo on 09/03/2025.
//

#pragma once
#include "portal/application/module/module.h"

// TODO: uncouple the renderer from vulkan
#include <vulkan/vulkan.hpp>

#include "portal/application/configuration.h"
#include "portal/application/vulkan/queue.h"


namespace portal
{
namespace gui
{
    class Gui;
}

namespace scene_graph
{
    class Scene;
}

class Window;


namespace vulkan
{
    class Stats;
    class RenderTarget;
    class PhysicalDevice;
    class DebugUtils;
    class Device;
    class Instance;

    namespace rendering
    {
        class RenderPipeline;
        class RenderContext;
    }
}

class Renderer : public portal::Module
{
public:
    Renderer();
    ~Renderer() override;

    Configuration& get_configuration();
    vulkan::rendering::RenderContext& get_render_context();
    [[nodiscard]] const vulkan::rendering::RenderContext& get_render_context() const;
    [[nodiscard]] bool has_render_context() const;
    [[nodiscard]] const std::vector<Hook>& get_hooks() const override;

    void on_start(const Configuration& config, debug::DebugInfo& debug_info) override;
    void on_close() override;

    void on_resize(uint32_t width, uint32_t) override;
    void on_update(float delta_time) override;

    void on_error() override;
    bool has_tag(TagID id) const override;

protected:
    /**
     * @brief Create the Vulkan device used by this sample
     * @note Can be overridden to implement custom device creation
     */
    virtual std::unique_ptr<vulkan::Device> create_device(vulkan::PhysicalDevice& gpu);

    /**
     * @brief Create the Vulkan instance used by this sample
     * @note Can be overridden to implement custom instance creation
     */
    virtual std::unique_ptr<vulkan::Instance> create_instance();

    /**
     * @brief Override this to customise the creation of the render_context
     */
    virtual void create_render_context();

    /**
     * @brief Prepares the render target and draws to it, calling draw_renderpass
     * @param command_buffer The command buffer to record the commands to
     * @param render_target The render target that is being drawn to
     */
    virtual void draw(vulkan::CommandBuffer& command_buffer, vulkan::RenderTarget& render_target);

    /**
     * @brief Samples should override this function to draw their interface
     */
    virtual void draw_gui();

    /**
     * @brief Starts the render pass, executes the render pipeline, and then ends the render pass
     * @param command_buffer The command buffer to record the commands to
     * @param render_target The render target that is being drawn to
     */
    virtual void draw_renderpass(vulkan::CommandBuffer& command_buffer, vulkan::RenderTarget& render_target);

    /**
     * @brief Override this to customise the creation of the swapchain and render_context
     */
    virtual void prepare_render_context();

    /**
     * @brief Triggers the render pipeline, it can be overridden by samples to specialize their rendering logic
     * @param command_buffer The command buffer to record the commands to
     */
    virtual void render(vulkan::CommandBuffer& command_buffer);

    /**
     * @brief Request features from the gpu based on what is supported
     */
    virtual void request_gpu_features(vulkan::PhysicalDevice& gpu);

    /**
     * @brief Resets the stats view max values for high demanding configs
     *        Should be overridden by the samples since they
     *        know which configuration is resource demanding
     */
    virtual void reset_stats_view();

    /**
     * @brief Updates the debug window, samples can override this to insert their own data elements
     */
    virtual void update_debug_window();

    /**
     * @brief Add a sample-specific device extension
     * @param extension The extension name
     * @param optional (Optional) Whether the extension is optional
     */
    void add_device_extension(const char* extension, bool optional = false);

    /**
     * @brief Add a sample-specific instance extension
     * @param extension The extension name
     * @param optional (Optional) Whether the extension is optional
     */
    void add_instance_extension(const char* extension, bool optional = false);

    /**
     * @brief Add a sample-specific instance layer
     * @param layer The layer name
     * @param optional (Optional) Whether the extension is optional
     */
    void add_instance_layer(const char* layer, bool optional = false);

    /**
     * @brief Add a sample-specific layer setting
     * @param layer_setting The layer setting
     */
    void add_layer_setting(const vk::LayerSettingEXT& layer_setting);

    void create_gui(const Window& window, const vulkan::Stats* stats = nullptr, const float font_size = 21.0f, bool explicit_update = false);

    /**
     * @brief A helper to create a render context
     */
    void create_render_context(const std::vector<vk::SurfaceFormatKHR>& surface_priority_list);

    vulkan::Device& get_device();
    const vulkan::Device& get_device() const;
    gui::Gui& get_gui();
    const gui::Gui& get_gui() const;
    vulkan::Instance& get_instance();
    const vulkan::Instance& get_instance() const;
    vulkan::rendering::RenderPipeline& get_render_pipeline();
    const vulkan::rendering::RenderPipeline& get_render_pipeline() const;
    scene_graph::Scene& get_scene();
    vulkan::Stats& get_stats();
    vk::SurfaceKHR get_surface() const;
    std::vector<vk::SurfaceFormatKHR>& get_surface_priority_list();
    const std::vector<vk::SurfaceFormatKHR>& get_surface_priority_list() const;

    bool has_device() const;
    bool has_instance() const;
    bool has_gui() const;
    bool has_render_pipeline() const;
    bool has_scene();

    /**
     * @brief Loads the scene
     *
     * @param path The path of the glTF file
     */
    void load_scene(const std::string& path);

    /**
     * @brief Set the Vulkan API version to request at instance creation time
     */
    void set_api_version(uint32_t requested_api_version);


    /**
     * @brief Sets whether or not the first graphics queue should have higher priority than other queues.
     * Very specific feature which is used by async compute samples.
     * Needs to be called before prepare().
     * @param enable If true, present queue will have prio 1.0 and other queues have prio 0.5.
     * Default state is false, where all queues have 0.5 priority.
     */
    void set_high_priority_graphics_queue_enable(bool enable);

    void set_render_context(std::unique_ptr<vulkan::rendering::RenderContext>&& render_context);

    void set_render_pipeline(std::unique_ptr<vulkan::rendering::RenderPipeline>&& render_pipeline);

    /**
     * @brief Update GUI
     * @param delta_time
     */
    void update_gui(float delta_time);

    /**
     * @brief Update scene
     * @param delta_time
     */
    void update_scene(float delta_time);

    /**
     * @brief Update counter values
     * @param delta_time
     */
    void update_stats(float delta_time);

    /**
     * @brief Set viewport and scissor state in command buffer for a given extent
     */
    static void set_viewport_and_scissor(const vulkan::CommandBuffer& command_buffer, const vk::Extent2D& extent);

private:
    void create_render_context_impl(const std::vector<vk::SurfaceFormatKHR>& surface_priority_list);
    void draw_impl(vulkan::CommandBuffer& command_buffer, vulkan::RenderTarget& render_target);
    void draw_renderpass_impl(vulkan::CommandBuffer& command_buffer, vulkan::RenderTarget& render_target);
    void render_impl(vulkan::CommandBuffer& command_buffer);
    static void set_viewport_and_scissor_impl(const vulkan::CommandBuffer& command_buffer, const vk::Extent2D& extent);

    /**
     * @brief Get sample-specific device extensions.
     *
     * @return Map of device extensions and whether or not they are optional. Default is empty map.
     */
    const std::unordered_map<const char*, bool>& get_device_extensions() const;

    /**
     * @brief Get sample-specific instance extensions.
     *
     * @return Map of instance extensions and whether or not they are optional. Default is empty map.
     */
    const std::unordered_map<const char*, bool>& get_instance_extensions() const;

    /**
     * @brief Get sample-specific instance layers.
     *
     * @return Map of instance layers and whether or not they are optional. Default is empty map.
     */
    const std::unordered_map<const char*, bool>& get_instance_layers() const;

    /**
     * @brief Get sample-specific layer settings.
     *
     * @return Vector of layer settings. Default is empty vector.
     */
    const std::vector<vk::LayerSettingEXT>& get_layer_settings() const;

private:
    std::unique_ptr<vulkan::Instance> instance;
    std::unique_ptr<vulkan::Device> device;

    std::unique_ptr<vulkan::rendering::RenderContext> render_context;
    std::unique_ptr<vulkan::rendering::RenderPipeline> render_pipeline;

    std::shared_ptr<scene_graph::Scene> scene;

    std::unique_ptr<gui::Gui> gui;
    std::unique_ptr<vulkan::Stats> stats;
    static constexpr float STATS_VIEW_RESET_TIME{10.0f}; // 10 seconds

    vk::SurfaceKHR surface{};

    // TODO: Uncouple the renderer from vulkan
    /**
     * @brief A list of surface formats in order of priority (vector[0] has high priority, vector[size-1] has low priority)
     */
    std::vector<vk::SurfaceFormatKHR> surface_priority_list = {
        {vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
        {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}
    };

    /**
     * @brief The configuration of the sample
     */
    Configuration configuration{};

    /** @brief Set of device extensions to be enabled for this example and whether they are optional (must be set in the derived constructor) */
    std::unordered_map<const char*, bool> device_extensions;

    /** @brief Set of instance extensions to be enabled for this example and whether they are optional (must be set in the derived constructor) */
    std::unordered_map<const char*, bool> instance_extensions;

    /** @brief Set of instance layers to be enabled for this example and whether they are optional (must be set in the derived constructor) */
    std::unordered_map<const char*, bool> instance_layers;

    /** @brief Vector of layer settings to be enabled for this example (must be set in the derived constructor) */
    std::vector<vk::LayerSettingEXT> layer_settings;

    /** @brief The Vulkan API version to request for this sample at instance creation time */
    uint32_t api_version = VK_API_VERSION_1_0;

    /** @brief Whether we want a high priority graphics queue. */
    bool high_priority_graphics_queue{false};

    std::unique_ptr<vulkan::DebugUtils> debug_utils;
};
} // portal
