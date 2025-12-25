
.. _program_listing_file_portal_engine_renderer_material_material.h:

Program Listing for File material.h
===================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_material_material.h>` (``portal\engine\renderer\material\material.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include "portal/engine/resources/resources/resource.h"
   #include "portal/engine/resources/resource_reference.h"
   #include "portal/core/reflection/property.h"
   #include "portal/core/reflection/property_concepts.h"
   #include "portal/core/strings/string_id.h"
   #include "portal/engine/renderer/shaders/shader.h"
   #include "portal/engine/renderer/image/texture.h"
   
   namespace portal::renderer
   {
   class ImageView;
   class Image;
   
   struct MaterialProperties
   {
       StringId id;
       Reference<ShaderVariant> shader;
   
       size_t set_start_index = 0;
       size_t set_end_index = std::numeric_limits<size_t>::max();
   
       Reference<Texture> default_texture;
   };
   
   class Material : public Resource
   {
   public:
       DECLARE_RESOURCE(ResourceType::Material);
   
       explicit Material(const StringId& id) : Resource(id) {}
   
       template <typename T> requires std::integral<std::remove_const_t<T>> || std::floating_point<std::remove_const_t<T>>
       void set(const StringId bind_point, const T& t)
       {
           set_property(
               bind_point,
               reflection::Property{
                   Buffer{const_cast<void*>(static_cast<const void*>(&t)), sizeof(T)},
                   reflection::get_property_type<std::remove_const_t<T>>(),
                   reflection::PropertyContainerType::scalar,
                   1
               }
           );
       }
   
       template <reflection::IsVec T>
       void set(const StringId bind_point, const T& t)
       {
           set_property(
               bind_point,
               reflection::Property{
                   Buffer{&t, sizeof(T)},
                   reflection::get_property_type<typename T::value_type>(),
                   reflection::PropertyContainerType::vector,
                   T::length()
               }
           );
       }
   
       template <reflection::IsMatrix T>
       void set(const StringId bind_point, const T& t)
       {
           set_property(
               bind_point,
               reflection::Property{
                   Buffer{&t, sizeof(T)},
                   reflection::get_property_type<typename T::value_type>(),
                   reflection::PropertyContainerType::matrix,
                   T::length() * T::length()
               }
           );
       }
   
       virtual void set(StringId bind_point, const ResourceReference<Texture>& texture) = 0;
       virtual void set(StringId bind_point, const Reference<Texture>& texture) = 0;
       virtual void set(StringId bind_point, const Reference<Image>& image) = 0;
       virtual void set(StringId bind_point, const Reference<ImageView>& image) = 0;
   
       template <typename T> requires std::integral<std::remove_const_t<T>> || std::floating_point<std::remove_const_t<T>>
       T& get(const StringId bind_point)
       {
           auto prop = reflection::Property{
               reflection::get_property_type<std::remove_const_t<T>>(),
               reflection::PropertyContainerType::scalar,
               1
           };
           get_property(bind_point, prop);
           return *prop.value.as<T*>();
       }
   
       template <reflection::IsVec T>
       T& get(const StringId bind_point)
       {
           auto prop = reflection::Property{
               reflection::get_property_type<typename T::value_type>(),
               reflection::PropertyContainerType::vector,
               T::length()
           };
           get_property(bind_point, prop);
           return *prop.value.as<T*>();
       }
   
       template <reflection::IsMatrix T>
       T& get(const StringId bind_point)
       {
           auto prop = reflection::Property{
               reflection::get_property_type<typename T::value_type>(),
               reflection::PropertyContainerType::matrix,
               T::length() * T::length()
           };
           get_property(bind_point, prop);
           return *prop.value.as<T*>();
       }
   
       virtual Reference<Texture> get_texture(const StringId bind_point) = 0;
       virtual Reference<Image> get_image(const StringId bind_point) = 0;
       virtual Reference<ImageView> get_image_view(const StringId bind_point) = 0;
   
       virtual Reference<ShaderVariant> get_shader() = 0;
   
   protected:
       virtual void set_property(StringId bind_point, const reflection::Property& property) = 0;
       virtual bool get_property(StringId bind_point, reflection::Property& property) const = 0;
   };
   }
