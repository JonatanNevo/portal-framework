
.. _program_listing_file_portal_engine_renderer_render_target_render_target.h:

Program Listing for File render_target.h
========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_render_target_render_target.h>` (``portal\engine\renderer\render_target\render_target.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <glaze/reflection/get_name.hpp>
   
   #include "portal/engine/reference.h"
   #include "portal/engine/renderer/image/image_types.h"
   
   namespace portal::renderer
   {
   enum class BlendMode
   {
       None,
       OneZero,
       SrcAlphaOneMinusSrcAlpha,
       Additive,
       ZeroSrcColor
   };
   
   enum class AttachmentLoadOperator
   {
       Inherit,
       Clear,
       Load
   };
   
   struct AttachmentTextureProperty
   {
       ImageFormat format{};
       bool blend = true;
       BlendMode blend_mode = BlendMode::SrcAlphaOneMinusSrcAlpha;
       AttachmentLoadOperator load_operator = AttachmentLoadOperator::Inherit;
   };
   
   struct AttachmentProperties
   {
       // A list of attachments, depth attachment is always last
       std::vector<AttachmentTextureProperty> attachment_images;
   
       // TODO: move depth attachment out of the vector as an optional member?
   
       // Master switch (individual attachments can be disabled in RenderTargetTextureProperties)
       bool blend = true;
       // None means use BlendMode in RenderTargetTextureProperties
       BlendMode blend_mode = BlendMode::None;
   };
   
   
   struct RenderTargetProperties
   {
       float scale = 1.f;
       size_t width = 0;
       size_t height = 0;
       glm::vec4 clear_color = {0.0f, 0.0f, 0.0f, 1.0f};
       float depth_clear_value = 0.f;
       bool clear_color_on_load = true;
       bool clear_depth_on_load = true;
   
       AttachmentProperties attachments;
       uint32_t samples = 1; // multisampling
   
       // Will it be used for transfer ops?
       bool transfer = false;
   
       StringId name;
   };
   
   class RenderTarget
   {
   public:
       virtual ~RenderTarget() = default;
   
       virtual void resize(size_t width, size_t height, bool force_recreate) = 0;
   
       [[nodiscard]] virtual size_t get_width() const = 0;
       [[nodiscard]] virtual size_t get_height() const = 0;
   
       [[nodiscard]] virtual size_t get_color_attachment_count() const = 0;
       [[nodiscard]] virtual bool has_depth_attachment() const = 0;
   
       [[nodiscard]] virtual const RenderTargetProperties& get_properties() const = 0;
   };
   } // portal
