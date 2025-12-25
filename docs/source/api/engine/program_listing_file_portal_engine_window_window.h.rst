
.. _program_listing_file_portal_engine_window_window.h:

Program Listing for File window.h
=================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_window_window.h>` (``portal\engine\window\window.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <filesystem>
   
   #include "portal/engine/reference.h"
   #include "portal/core/events/event_handler.h"
   #include "portal/core/strings/string_id.h"
   #include "portal/engine/renderer/image/texture.h"
   #include "portal/engine/resources/resource_reference.h"
   
   namespace portal
   {
   class InputEventConsumer;
   class WindowEventConsumer;
   }
   
   namespace portal
   {
   namespace renderer
   {
       namespace vulkan
       {
           class VulkanContext;
       }
   
       class Surface;
   }
   
   struct WindowExtent
   {
       size_t width;
       size_t height;
   };
   
   enum class WindowMode
   {
       Headless,
       Fullscreen,
       FullscreenBorderless,
       Default,
   };
   
   struct WindowProperties
   {
       StringId title = STRING_ID("Portal");
       WindowExtent extent{1280, 720};
       renderer::Texture* icon;
   
       WindowMode mode = WindowMode::Default;
       bool resizeable = true;
       bool vsync = true;
       bool decorated = true;
   
       size_t requested_frames_in_flight = 3;
   };
   
   struct CallbackConsumers
   {
       std::reference_wrapper<WindowEventConsumer> window;
       std::reference_wrapper<InputEventConsumer> input;
   };
   
   class Window : public EventHandler
   {
   public:
       Window(const WindowProperties& properties, const CallbackConsumers& consumers);
   
       virtual void process_events() = 0;
   
       [[nodiscard]] virtual bool should_close() const = 0;
   
       virtual void close() = 0;
   
       WindowExtent resize(const WindowExtent& requested_extent);
   
       [[nodiscard]] virtual Reference<renderer::Surface> create_surface(const renderer::vulkan::VulkanContext& context) = 0;
   
       [[nodiscard]] virtual float get_dpi_factor() const = 0;
   
       [[nodiscard]] virtual float get_content_scale_factor() const;
   
       virtual void maximize() = 0;
       virtual void center_window() = 0;
   
       virtual void set_vsync(bool enable) = 0;
       virtual void set_resizeable(bool enable) = 0;
       virtual void set_title(StringId title) = 0;
   
       [[nodiscard]] virtual glm::vec2 get_position() const = 0;
       [[nodiscard]] size_t get_width() const { return properties.extent.width; };
       [[nodiscard]] size_t get_height() const { return properties.extent.height; };
       [[nodiscard]] WindowExtent get_extent() const { return properties.extent; }
       [[nodiscard]] StringId get_title() const { return properties.title; }
       [[nodiscard]] bool is_resizeable() const { return properties.resizeable; }
       [[nodiscard]] WindowMode get_mode() const { return properties.mode; }
       [[nodiscard]] bool is_vsynced() const { return properties.vsync; }
   
       [[nodiscard]] const WindowProperties& get_properties() const { return properties; }
   
   protected:
       WindowProperties properties;
       CallbackConsumers consumers;
   };
   } // portal
