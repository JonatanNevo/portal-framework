//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/reference.h"

#include "portal/engine/resources/resources/resource.h"
#include "portal/core/reflection/property.h"
#include "portal/core/reflection/concepts.h"
#include "portal/engine/strings/string_id.h"
#include "portal/engine/renderer/shaders/shader.h"
#include "portal/engine/renderer/image/texture.h"

namespace portal::renderer
{
class ImageView;
class Image;

struct MaterialSpecification
{
    StringId id;
    Ref<ShaderVariant> shader;

    size_t set_start_index = 0;
    size_t set_end_index = std::numeric_limits<size_t>::max();

    Ref<Texture> default_texture;
};

class Material : public Resource
{
public:
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

    virtual void set(const StringId bind_point, Ref<Texture> texture) = 0;
    virtual void set(const StringId bind_point, Ref<renderer::Image> image) = 0;
    virtual void set(const StringId bind_point, Ref<renderer::ImageView> image) = 0;

    template <typename T> requires std::integral<std::remove_const_t<T>> || std::floating_point<std::remove_const_t<T>>
    T& get(const StringId bind_point)
    {
        auto prop = reflection::Property{
            reflection::get_property_type<std::remove_const_t<T>>(),
            reflection::PropertyContainerType::scalar,
            1
        };
        get_property(bind_point, prop);
        return *reinterpret_cast<T*>(prop.value.data());
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
        return *reinterpret_cast<T*>(prop.value.data());
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
        return *reinterpret_cast<T*>(prop.value.data());
    }

    virtual Ref<Texture> get_texture(const StringId bind_point) = 0;
    virtual Ref<renderer::Image> get_image(const StringId bind_point) = 0;
    virtual Ref<renderer::ImageView> get_image_view(const StringId bind_point) = 0;

    virtual Ref<ShaderVariant> get_shader() = 0;
    virtual StringId get_id() = 0;

protected:
    virtual void set_property(StringId bind_point, const reflection::Property& property) = 0;
    virtual bool get_property(StringId bind_point, reflection::Property& property) const = 0;
};

}
