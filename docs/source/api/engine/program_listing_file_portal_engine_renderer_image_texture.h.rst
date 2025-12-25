
.. _program_listing_file_portal_engine_renderer_image_texture.h:

Program Listing for File texture.h
==================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_image_texture.h>` (``portal\engine\renderer\image\texture.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include "portal/core/buffer.h"
   #include "portal/engine/reference.h"
   #include "portal/engine/renderer/renderer_resource.h"
   #include "portal/engine/renderer/image/image_types.h"
   
   namespace portal::renderer
   {
   class Texture : public RendererResource
   {
   public:
       const static StringId MISSING_TEXTURE_ID;
       const static StringId WHITE_TEXTURE_ID;
       const static StringId BLACK_TEXTURE_ID;
   
   public:
       explicit Texture(const StringId& id) : RendererResource(id) {};
   
       [[nodiscard]] virtual ImageFormat get_format() const = 0;
   
       [[nodiscard]] virtual size_t get_width() const = 0;
       [[nodiscard]] virtual size_t get_height() const = 0;
       [[nodiscard]] virtual size_t get_depth() const = 0;
   
       [[nodiscard]] virtual glm::uvec3 get_size() const = 0;
   
       [[nodiscard]] virtual uint32_t get_mip_level_count() const = 0;
       [[nodiscard]] virtual glm::uvec3 get_mip_size(uint32_t mip) const = 0;
   
       virtual void resize(const glm::uvec3& size) = 0;
       virtual void resize(size_t width, size_t height, size_t depth) = 0;
   
       [[nodiscard]] virtual Reference<Image> get_image() const = 0;
       [[nodiscard]] virtual Buffer get_buffer() const = 0;
       virtual Buffer get_writeable_buffer() = 0;
   
       virtual bool loaded() const = 0;
   
       [[nodiscard]] virtual TextureType get_type() const = 0;
   
       static ResourceType static_type() { return ResourceType::Texture; }
   };
   }
