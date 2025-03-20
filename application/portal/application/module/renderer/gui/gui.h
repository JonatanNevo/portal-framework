//
// Created by Jonatan Nevo on 15/03/2025.
//

#pragma once
#include <filesystem>

#include <imgui.h>
#include <map>
#include <portal/core/file_system.h>


#include "portal/application/module/renderer/gui/drawer.h"
#include "portal/application/vulkan/stats/stats.h"

namespace portal::vulkan
{
class BufferAllocation;
}

namespace portal
{
class InputEvent;

namespace debug
{
    class DebugInfo;
}

class Renderer;
}

namespace portal::vulkan
{
class Stats;
}

namespace portal::gui
{
/**
 * @brief Helper structure for fonts loaded from TTF
 */
struct Font
{
    /**
     * @brief Constructor
     * @param path The path of the font file
     * @param size The font size, scaled by DPI
     */
    Font(const std::filesystem::path& path, const float size): data(filesystem::read_file_binary(path)), name(path.filename().string()), size(size)
    {
        // Keep ownership of the font data to avoid a double delete
        ImFontConfig font_config{};
        font_config.FontDataOwnedByAtlas = false;

        if (size < 1.0f)
            this->size = 20.0f;

        const ImGuiIO& io = ImGui::GetIO();
        handle = io.Fonts->AddFontFromMemoryTTF(data.data(), static_cast<int>(data.size()), size, &font_config);
    }

    std::vector<uint8_t> data;
    ImFont* handle = nullptr;
    std::string name;
    float size = 0.0f;
};


/**
 * @brief Vulkan helper class for Dear ImGui
 */
class Gui
{
public:
    /**
    * @brief Helper class for drawing statistics
    */
    class StatsView
    {
    public:
        /**
         * @brief Constructs a StatsView
         * @param stats Const pointer to the Stats data object; may be null
         */
        StatsView(const vulkan::Stats* stats);

        /**
         * @brief Resets the max values for the stats
         *        which do not have a fixed max
         */
        void reset_max_values();

        /**
         * @brief Resets the max value for a specific stat
         */
        void reset_max_value(vulkan::stats::StatIndex index);

    public:
        std::map<vulkan::stats::StatIndex, vulkan::stats::StatGraphData> graph_map;
        float graph_height = 50.0f;
        float top_padding = 1.1f;
    };

public:
    // The name of the default font file to use
    static const std::string default_font;
    // Used to show/hide the GUI
    static bool visible;

public:
    /**
     * @brief Initializes the HPPGui
     * @param renderer A render context
     * @param window A Window object from which to draw DPI and content scaling information
     * @param stats A statistics object (null if no statistics are used)
     * @param font_size The font size
     * @param explicit_update If true, update buffers every frame
     */
    Gui(
        Renderer& renderer,
        const Window& window,
        const vulkan::Stats* stats = nullptr,
        float font_size = 21.0f,
        bool explicit_update = false
    );

    /**
     * @brief Destroys the HPPGui
     */
    ~Gui();

    void prepare(vk::PipelineCache pipeline_cache, vk::RenderPass render_pass, const std::vector<vk::PipelineShaderStageCreateInfo>& shader_stages);

    /**
     * @brief Handles resizing of the window
     * @param width New width of the window
     * @param height New height of the window
     */
    void resize(uint32_t width, uint32_t height) const;

    /**
     * @brief Starts a new ImGui frame
     *        to be called before drawing any window
     */
    void new_frame() const;

    /**
     * @brief Updates the Gui
     * @param delta_time Time passed since last update
     */
    void update(float delta_time);

    bool update_buffers();

    /**
     * @brief Draws the Gui
     * @param command_buffer Command buffer to register draw-commands
     */
    void draw(vulkan::CommandBuffer& command_buffer);

    /**
     * @brief Shows an overlay top window with app info and maybe stats
     * @param app_name Application name
     * @param stats Statistics to show (can be null)
     * @param debug_info Debug info to show (can be null)
     */
    void show_top_window(const std::string& app_name, const vulkan::Stats* stats = nullptr, const debug::DebugInfo* debug_info = nullptr);

    /**
     * @brief Shows the ImGui Demo window
     */
    void show_demo_window() const;

    /**
     * @brief Shows a child with app info
     * @param app_name Application name
     */
    void show_app_info(const std::string& app_name) const;

    /**
     * @brief Shows a moveable window with debug information
     * @param debug_info The object holding the data fields to be displayed
     * @param position The absolute position to set
     */
    void show_debug_window(const debug::DebugInfo& debug_info, const ImVec2& position);

    /**
     * @brief Shows a child with statistics
     * @param stats Statistics to show
     */
    void show_stats(const vulkan::Stats& stats);

    /**
     * @brief Shows an options windows, to be filled by the sample,
     *        which will be positioned at the top
     * @param body ImGui commands defining the body of the window
     * @param lines The number of lines of text to draw in the window
     *        These will help the gui to calculate the height of the window
     */
    void show_options_window(const std::function<void()>& body, uint32_t lines = 3) const;

    void show_simple_window(const std::string& name, uint32_t last_fps, const std::function<void()>& body) const;

    bool input_event(const InputEvent& input_event);

    /**
     * @return The stats view
     */
    [[nodiscard]] const StatsView& get_stats_view() const;
    Drawer& get_drawer();
    [[nodiscard]] Font const& get_font(const std::string& font_name = Gui::default_font) const;
    [[nodiscard]] bool is_debug_view_active() const;

private:
    /**
     * @brief Updates Vulkan buffers
     * @param command_buffer Command buffer to draw into
     * @return Vertex buffer allocation
     */
    vulkan::BufferAllocation update_buffers(vulkan::CommandBuffer& command_buffer) const;

private:
    /**
     * @brief Helper class for rendering debug statistics in the GUI
     */
    struct DebugView
    {
        bool active = false;
        uint32_t max_fields = 8;
        float label_column_width = 0;
        float scale = 1.7f;
    };

    struct PushConstBlock
    {
        glm::vec2 scale;
        glm::vec2 translate;
    };

private:
    /**
     * @brief Block size of a buffer pool in kilobytes
     */
    static constexpr uint32_t BUFFER_POOL_BLOCK_SIZE = 256;

    static const double press_time_ms;
    static const float overlay_alpha;
    static const ImGuiWindowFlags common_flags;
    static const ImGuiWindowFlags options_flags;
    static const ImGuiWindowFlags info_flags;

private:
    PushConstBlock push_const_block;
    Renderer& renderer;
    std::shared_ptr<vulkan::Buffer> vertex_buffer;
    std::shared_ptr<vulkan::Buffer> index_buffer;
    size_t last_vertex_buffer_size = 0;
    size_t last_index_buffer_size = 0;
    float content_scale_factor = 1.0f; // Scale factor to apply due to a difference between the window and GL pixel sizes
    float dpi_factor = 1.0f;           // Scale factor to apply to the size of gui elements (expressed in dp)
    bool explicit_update = false;
    Drawer drawer;
    std::vector<Font> fonts;
    std::unique_ptr<vulkan::Image> font_image;
    std::unique_ptr<vulkan::ImageView> font_image_view;
    std::unique_ptr<vulkan::Sampler> sampler;
    vulkan::PipelineLayout* pipeline_layout = nullptr;
    StatsView stats_view;
    DebugView debug_view;
    vk::DescriptorPool descriptor_pool = nullptr;
    vk::DescriptorSetLayout descriptor_set_layout = nullptr;
    vk::DescriptorSet descriptor_set = nullptr;
    vk::Pipeline pipeline = nullptr;
    Timer timer; // Used to measure duration of input events
    bool prev_visible = true;
    bool two_finger_tap = false; // Whether the GUI has detected a multitouch gesture
    bool show_graph_file_output = false;
    uint32_t subpass = 0;
};
} // portal
