//
// Created by Jonatan Nevo on 09/03/2025.
//

#include "renderer.h"

#include "portal/application/vulkan/device.h"


namespace portal
{
Renderer::Renderer(): Module("Renderer")
{}

Renderer::~Renderer()
{
    if (device)
        device->get_handle().waitIdle();

    scene.reset();
    stats.reset();
    gui.reset();
    render_context.reset();
    device.reset();

    if (surface)
        instance->get_handle().destroySurfaceKHR(surface);

    instance.reset();
}

Configuration& Renderer::get_configuration()
{

}

vulkan::rendering::RenderContext& Renderer::get_render_context() {
}

const vulkan::rendering::RenderContext& Renderer::get_render_context() const {
}

bool Renderer::has_render_context() const {
}

const std::vector<Module::Hook>& Renderer::get_hooks() const {
}

void Renderer::on_start(const Configuration& config, debug::DebugInfo& debug_info) {
}

void Renderer::on_close() {
}

void Renderer::on_resize(uint32_t width, uint32_t) {
}

void Renderer::on_update(float delta_time) {
}

void Renderer::on_error() {
}

bool Renderer::has_tag(TagID id) const {
}

std::unique_ptr<vulkan::Device> Renderer::create_device(vulkan::PhysicalDevice& gpu)
{
r
}

std::unique_ptr<vulkan::Instance> Renderer::create_instance() {
}

void Renderer::create_render_context() {
}

void Renderer::draw(vulkan::CommandBuffer& command_buffer, vulkan::RenderTarget& render_target) {
}

void Renderer::draw_gui() {
}

void Renderer::draw_renderpass(vulkan::CommandBuffer& command_buffer, vulkan::RenderTarget& render_target) {
}

void Renderer::prepare_render_context() {
}

void Renderer::render(vulkan::CommandBuffer& command_buffer) {
}

void Renderer::request_gpu_features(vulkan::PhysicalDevice& gpu) {
}

void Renderer::reset_stats_view() {
}

void Renderer::update_debug_window() {
}

void Renderer::add_device_extension(const char* extension, bool optional)
{
    device_extensions[extension] = optional;
}

void Renderer::add_instance_extension(const char* extension, bool optional)
{
    instance_extensions[extension] = optional;
}

void Renderer::add_instance_layer(const char* layer, bool optional) {
}

void Renderer::add_layer_setting(const vk::LayerSettingEXT& layer_setting)
{
    layer_settings.push_back(layer_setting);
}

void Renderer::create_gui(const Window& window, const vulkan::Stats* stats, const float font_size, bool explicit_update) {
}

void Renderer::create_render_context(const std::vector<vk::SurfaceFormatKHR>& surface_priority_list) {
}

vulkan::Device& Renderer::get_device() {
}

const vulkan::Device& Renderer::get_device() const {
}

gui::Gui& Renderer::get_gui() {
}

const gui::Gui& Renderer::get_gui() const {
}

vulkan::Instance& Renderer::get_instance() {
}

const vulkan::Instance& Renderer::get_instance() const {
}

vulkan::rendering::RenderPipeline& Renderer::get_render_pipeline() {
}

const vulkan::rendering::RenderPipeline& Renderer::get_render_pipeline() const {
}

scene_graph::Scene& Renderer::get_scene() {
}

vulkan::Stats& Renderer::get_stats() {
}

vk::SurfaceKHR Renderer::get_surface() const {
}

std::vector<vk::SurfaceFormatKHR>& Renderer::get_surface_priority_list() {
}

const std::vector<vk::SurfaceFormatKHR>& Renderer::get_surface_priority_list() const {
}

bool Renderer::has_device() const {
}

bool Renderer::has_instance() const {
}

bool Renderer::has_gui() const {
}

bool Renderer::has_render_pipeline() const {
}

bool Renderer::has_scene() {
}

void Renderer::load_scene(const std::string& path) {
}

void Renderer::set_api_version(uint32_t requested_api_version) {
}

void Renderer::set_high_priority_graphics_queue_enable(bool enable) {
}

void Renderer::set_render_context(std::unique_ptr<vulkan::rendering::RenderContext>&& render_context) {
}

void Renderer::set_render_pipeline(std::unique_ptr<vulkan::rendering::RenderPipeline>&& render_pipeline) {
}

void Renderer::update_gui(float delta_time) {
}

void Renderer::update_scene(float delta_time) {
}

void Renderer::update_stats(float delta_time) {
}

void Renderer::set_viewport_and_scissor(const vulkan::CommandBuffer& command_buffer, const vk::Extent2D& extent) {
}

void Renderer::create_render_context_impl(const std::vector<vk::SurfaceFormatKHR>& surface_priority_list) {
}

void Renderer::draw_impl(vulkan::CommandBuffer& command_buffer, vulkan::RenderTarget& render_target) {
}

void Renderer::draw_renderpass_impl(vulkan::CommandBuffer& command_buffer, vulkan::RenderTarget& render_target) {
}

void Renderer::render_impl(vulkan::CommandBuffer& command_buffer) {
}

void Renderer::set_viewport_and_scissor_impl(const vulkan::CommandBuffer& command_buffer, const vk::Extent2D& extent) {
}

const std::unordered_map<const char*, bool>& Renderer::get_device_extensions() const {
}

const std::unordered_map<const char*, bool>& Renderer::get_instance_extensions() const {
}

const std::unordered_map<const char*, bool>& Renderer::get_instance_layers() const {
}

const std::vector<vk::LayerSettingEXT>& Renderer::get_layer_settings() const {
}
} // portal
